#pragma once

#include "orderbook/MemoryPool.h"
#include "orderbook/Order.h"
#include "orderbook/SideBook.h"
#include "orderbook/Types.h"

#include <deque>
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>

namespace ob {

struct Trade {
    std::string      resting_id;
    types::Price     resting_px;
    types::Quantity  traded_qty;
    std::string      incoming_id;
    types::Price     incoming_px;
};

class OrderBook {
public:
    explicit OrderBook(std::size_t pool_capacity = 1'000);

    Order* create_order(std::string id,
                        types::Price price,
                        types::Quantity qty,
                        types::Side side,
                        types::TimeInForce tif,
                        std::optional<types::Quantity> min_qty = std::nullopt);

    void cancel(const std::string& id);
    void modify(const std::string& id, types::Price price, types::Quantity qty,
                std::optional<types::Quantity> min_qty = std::nullopt);

    void process(Order& order);

    const SideBook& bids() const noexcept { return bids_; }
    const SideBook& asks() const noexcept { return asks_; }

    bool has_order(const std::string& id) const;

    using trade_callback = std::function<void(const Trade&)>;
    void set_trade_callback(trade_callback cb) { on_trade_ = std::move(cb); }

    void snapshot(std::ostream& os) const;

private:
    void match(Order& incoming, SideBook& opposite, SideBook& same);

    MemoryPool<Order> pool_;
    SideBook bids_;
    SideBook asks_;
    std::unordered_map<std::string, Order*, types::OrderIdHash> index_;
    trade_callback on_trade_;
};

} // namespace ob
