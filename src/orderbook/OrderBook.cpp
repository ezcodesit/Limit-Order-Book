#include "orderbook/OrderBook.h"

#include <algorithm>
#include <iostream>
#include <vector>

namespace ob {

OrderBook::OrderBook(std::size_t pool_capacity)
    : pool_(pool_capacity)
    , bids_(types::Side::Buy)
    , asks_(types::Side::Sell) {}

Order* OrderBook::create_order(std::string id,
                               types::Price price,
                               types::Quantity qty,
                               types::Side side,
                               types::TimeInForce tif,
                               std::optional<types::Quantity> min_qty) {
    if (index_.contains(id)) return nullptr;
    auto* order = pool_.create(std::move(id), price, qty, side, tif, min_qty);
    order->node.order = order;
    index_[order->id] = order;
    std::string order_id = order->id;
    process(*order);
    if (!index_.contains(order_id)) {
        return nullptr;
    }
    return index_[order_id];
}

void OrderBook::cancel(const std::string& id) {
    auto it = index_.find(id);
    if (it == index_.end()) return;
    Order* order = it->second;
    if (order->side == types::Side::Buy) bids_.remove(*order);
    else asks_.remove(*order);
    index_.erase(it);
    pool_.destroy(order);
}

void OrderBook::modify(const std::string& id, types::Price price, types::Quantity qty,
                       std::optional<types::Quantity> min_qty) {
    auto it = index_.find(id);
    if (it == index_.end()) return;
    Order* old = it->second;
    types::Side side = old->side;
    cancel(id);
    create_order(id, price, qty, side, types::TimeInForce::GFD, min_qty);
}

void OrderBook::process(Order& order) {
    match(order, order.side == types::Side::Buy ? asks_ : bids_,
          order.side == types::Side::Buy ? bids_ : asks_);
}

bool OrderBook::has_order(const std::string& id) const {
    return index_.contains(id);
}

void OrderBook::snapshot(std::ostream& os) const {
    os << "SELL:\n";
    std::vector<const PriceLevel*> asks;
    asks.reserve(asks_.empty() ? 0 : 64);
    asks_.for_each_level([&](const PriceLevel& level) { asks.push_back(&level); });
    std::sort(asks.begin(), asks.end(), [](auto* lhs, auto* rhs) {
        return lhs->price() < rhs->price();
    });
    for (const auto* level : asks) {
        os << level->price() << ' ' << level->total() << '\n';
    }

    os << "BUY:\n";
    std::vector<const PriceLevel*> bids;
    bids.reserve(bids_.empty() ? 0 : 64);
    bids_.for_each_level([&](const PriceLevel& level) { bids.push_back(&level); });
    std::sort(bids.begin(), bids.end(), [](auto* lhs, auto* rhs) {
        return lhs->price() > rhs->price();
    });
    for (const auto* level : bids) {
        os << level->price() << ' ' << level->total() << '\n';
    }
}

void OrderBook::match(Order& incoming, SideBook& opposite, SideBook& same) {
    auto available_qty = [&]() {
        types::Quantity total = 0;
        std::vector<const PriceLevel*> levels;
        opposite.for_each_level([&](const PriceLevel& level) {
            bool crosses = incoming.side == types::Side::Buy
                                ? incoming.price >= level.price()
                                : incoming.price <= level.price();
            if (crosses) levels.push_back(&level);
        });
        for (const auto* level : levels) total += level->total();
        return total;
    }();

    if (incoming.tif == types::TimeInForce::FOK && available_qty < incoming.quantity) {
        cancel(incoming.id);
        return;
    }

    if (incoming.min_qty && available_qty < *incoming.min_qty) {
        cancel(incoming.id);
        return;
    }

    Order* resting = nullptr;
    while (incoming.quantity > 0) {
        resting = opposite.best();
        if (!resting) break;
        bool price_cross = incoming.side == types::Side::Buy
                                ? incoming.price >= resting->price
                                : incoming.price <= resting->price;
        if (!price_cross) break;

        auto traded = std::min(incoming.quantity, resting->quantity);
        incoming.quantity -= traded;
        resting->quantity -= traded;
        opposite.on_fill(*resting, traded);

        if (on_trade_) {
            on_trade_(Trade{resting->id, resting->price, traded, incoming.id, incoming.price});
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
    } else {
        cancel(incoming.id);
    }
}

} // namespace ob
