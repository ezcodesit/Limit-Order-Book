#pragma once

#include "orderbook/MemoryPool.h"
#include "orderbook/Order.h"
#include "orderbook/SideBook.h"
#include "orderbook/Types.h"

#include <optional>
#include <ostream>
#include <vector>

namespace ob {

/**
 * @brief Lightweight trade report emitted for each match.
 */
struct Trade {
    types::OrderId   resting_id{types::invalid_order_id};
    types::Price     resting_px{0};
    types::Quantity  traded_qty{0};
    types::OrderId   incoming_id{types::invalid_order_id};
    types::Price     incoming_px{0};
};

/**
 * @brief Deterministic single-symbol order book with price-time priority.
 *
 * The book owns both bid and ask ladders, a memory pool for orders,
 * and a direct index from internal order IDs to `Order*`.
 */
class OrderBook {
public:
    /// Callback signature invoked for each executed trade.
    using trade_sink_t = void(*)(const Trade&, void*);

    /**
     * @brief Construct a book spanning the price window [min_price, max_price].
     * @param min_price Lowest price initially addressable.
     * @param max_price Highest price initially addressable.
     * @param pool_capacity Maximum number of simultaneously active orders.
     */
    OrderBook(types::Price min_price,
              types::Price max_price,
              std::size_t  pool_capacity = 1'000);

    /**
     * @brief Add an order to the book, matching immediately if possible.
     *
     * @return Pointer to the live order when it rests, or nullptr if it traded fully
     *         or could not be accepted (duplicate ID, pool exhausted, FOK rejection).
     */
    Order* create_order(types::OrderId id,
                        types::Price price,
                        types::Quantity qty,
                        types::Side side,
                        types::TimeInForce tif,
                        std::optional<types::Quantity> min_qty = std::nullopt);

    /// Cancel an order by its internal identifier (no-op if absent).
    void cancel(types::OrderId id);

    /**
     * @brief Modify an existing order by cancel+reenter semantics.
     *
     * @param id        Existing order identifier.
     * @param side      Replacement side.
     * @param price     Replacement price.
     * @param qty       Replacement quantity.
     * @param tif       Replacement time-in-force.
     * @param min_qty   Optional replacement minimum quantity.
     */
    void modify(types::OrderId id,
                types::Side side,
                types::Price price,
                types::Quantity qty,
                types::TimeInForce tif,
                std::optional<types::Quantity> min_qty = std::nullopt);

    /// @return True if the internal identifier currently maps to a live order.
    bool has_order(types::OrderId id) const;

    /// Lookup helper used by higher layers to inspect resting orders.
    const Order* find(types::OrderId id) const noexcept;

    /// Install a trade sink callback invoked for each match.
    void set_trade_sink(trade_sink_t sink, void* ctx) noexcept {
        trade_sink_ = sink;
        trade_ctx_  = ctx;
    }

    /// Emit a textual snapshot of the book to @p os.
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
