#include "orderbook/OrderBook.h"

#include <algorithm>
#include <iostream>
#include <vector>

namespace ob {

OrderBook::OrderBook(types::Price min_price,
                     types::Price max_price,
                     std::size_t  pool_capacity)
    : pool_(pool_capacity)
    , bids_(types::Side::Buy, min_price, max_price)
    , asks_(types::Side::Sell, min_price, max_price) {}

void OrderBook::ensure_index_capacity(types::OrderId id) {
    if (id >= id_index_.size()) {
        id_index_.resize(static_cast<std::size_t>(id + 1), nullptr);
    }
}

Order* OrderBook::create_order(types::OrderId id,
                               types::Price price,
                               types::Quantity qty,
                               types::Side side,
                               types::TimeInForce tif,
                               std::optional<types::Quantity> min_qty) {
    ensure_index_capacity(id);
    if (id_index_[id]) {
        return nullptr; // duplicate id
    }

    auto* order = pool_.create(id, price, qty, side, tif, min_qty);
    if (!order) return nullptr;

    order->node.order = order;
    id_index_[id]     = order;
    process(*order);
    if (!has_order(id)) {
        return nullptr;
    }
    return id_index_[id];
}

void OrderBook::cancel(types::OrderId id) {
    if (id >= id_index_.size()) return;
    Order* order = id_index_[id];
    if (!order) return;

    if (order->resting) {
        if (order->side == types::Side::Buy) bids_.remove(*order);
        else asks_.remove(*order);
    }

    id_index_[id] = nullptr;
    pool_.destroy(order);
}

void OrderBook::modify(types::OrderId id,
                       types::Side side,
                       types::Price price,
                       types::Quantity qty,
                       types::TimeInForce tif,
                       std::optional<types::Quantity> min_qty) {
    if (id >= id_index_.size()) return;
    Order* existing = id_index_[id];
    if (!existing) return;
    cancel(id);
    create_order(id, price, qty, side, tif, min_qty);
}

bool OrderBook::has_order(types::OrderId id) const {
    return id < id_index_.size() && id_index_[id] != nullptr;
}

const Order* OrderBook::find(types::OrderId id) const noexcept {
    if (id >= id_index_.size()) return nullptr;
    return id_index_[id];
}

void OrderBook::process(Order& order) {
    if (order.side == types::Side::Buy) {
        match(order, asks_, bids_);
    } else {
        match(order, bids_, asks_);
    }
}

void OrderBook::match(Order& incoming, SideBook& opposite, SideBook& same) {
    const auto available = opposite.available_to(incoming.price, incoming.side);
    if (incoming.tif == types::TimeInForce::FOK && available < incoming.quantity) {
        cancel(incoming.id);
        return;
    }
    if (incoming.has_min_qty && available < incoming.min_qty) {
        cancel(incoming.id);
        return;
    }

    while (incoming.quantity > 0) {
        Order* resting = opposite.best();
        if (!resting) break;
        bool price_cross = incoming.side == types::Side::Buy
                                ? incoming.price >= resting->price
                                : incoming.price <= resting->price;
        if (!price_cross) break;

        auto traded = std::min(incoming.quantity, resting->quantity);
        incoming.quantity -= traded;
        resting->quantity -= traded;
        opposite.on_fill(*resting, traded);

        if (trade_sink_) {
            trade_sink_(Trade{resting->id, resting->price, traded, incoming.id, incoming.price}, trade_ctx_);
        }

        if (resting->quantity == 0) {
            cancel(resting->id);
        }

        if (incoming.tif == types::TimeInForce::IOC) {
            cancel(incoming.id);
            return;
        }
    }

    if (incoming.quantity > 0 && incoming.tif == types::TimeInForce::GFD) {
        same.add(incoming);
        incoming.resting = true;
    } else {
        cancel(incoming.id);
    }
}

void OrderBook::snapshot(std::ostream& os) const {
    os << "SELL:\n";
    std::vector<const PriceLevel*> asks;
    asks.reserve(64);
    asks_.for_each_level([&](const PriceLevel& level) {
        if (level.total() > 0) asks.push_back(&level);
    });
    std::sort(asks.begin(), asks.end(), [](const PriceLevel* lhs, const PriceLevel* rhs) {
        return lhs->price() < rhs->price();
    });
    for (const auto* level : asks) {
        os << level->price() << ' ' << level->total() << '\n';
    }

    os << "BUY:\n";
    std::vector<const PriceLevel*> bids;
    bids.reserve(64);
    bids_.for_each_level([&](const PriceLevel& level) {
        if (level.total() > 0) bids.push_back(&level);
    });
    std::sort(bids.begin(), bids.end(), [](const PriceLevel* lhs, const PriceLevel* rhs) {
        return lhs->price() > rhs->price();
    });
    for (const auto* level : bids) {
        os << level->price() << ' ' << level->total() << '\n';
    }
}

} // namespace ob
