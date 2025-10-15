#pragma once

#include "orderbook/MemoryPool.h"
#include "orderbook/Order.h"
#include "orderbook/SideBook.h"
#include "orderbook/Types.h"

#include <optional>
#include <ostream>
#include <vector>

namespace ob {

struct Trade {
    types::OrderId   resting_id{types::invalid_order_id};
    types::Price     resting_px{0};
    types::Quantity  traded_qty{0};
    types::OrderId   incoming_id{types::invalid_order_id};
    types::Price     incoming_px{0};
};

class OrderBook {
public:
    using trade_sink_t = void(*)(const Trade&, void*);

    OrderBook(types::Price min_price,
              types::Price max_price,
              std::size_t  pool_capacity = 1'000);

    Order* create_order(types::OrderId id,
                        types::Price price,
                        types::Quantity qty,
                        types::Side side,
                        types::TimeInForce tif,
                        std::optional<types::Quantity> min_qty = std::nullopt);

    void cancel(types::OrderId id);
    void modify(types::OrderId id,
                types::Side side,
                types::Price price,
                types::Quantity qty,
                types::TimeInForce tif,
                std::optional<types::Quantity> min_qty = std::nullopt);

    bool has_order(types::OrderId id) const;
    const Order* find(types::OrderId id) const noexcept;

    void set_trade_sink(trade_sink_t sink, void* ctx) noexcept {
        trade_sink_ = sink;
        trade_ctx_  = ctx;
    }

    void snapshot(std::ostream& os) const;

private:
    void process(Order& order);
    void match(Order& incoming, SideBook& opposite, SideBook& same);
    void ensure_index_capacity(types::OrderId id);

    MemoryPool<Order> pool_;
    SideBook bids_;
    SideBook asks_;
    std::vector<Order*> id_index_;
    trade_sink_t        trade_sink_{nullptr};
    void*               trade_ctx_{nullptr};
};

} // namespace ob
