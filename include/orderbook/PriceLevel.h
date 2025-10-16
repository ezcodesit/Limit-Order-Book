#pragma once

#include "orderbook/IntrusiveList.h"
#include "orderbook/Order.h"
#include "orderbook/Types.h"

namespace ob {

/**
 * @brief Aggregates all resting orders at a single price.
 *
 * Each level maintains total resting quantity and a FIFO of orders to enforce
 * price-time priority within the level.
 */
class PriceLevel {
public:
    /// Construct a level optionally pre-initialised with @p price.
    explicit PriceLevel(types::Price price = 0) : price_(price) {}

    /// @return Price represented by this level.
    types::Price price() const noexcept { return price_; }

    /// @return Aggregate resting quantity at this price.
    types::Quantity total() const noexcept { return total_quantity_; }

    /// @return True when no orders currently rest at this level.
    bool empty() const noexcept { return orders_.empty(); }

    /// Set the level's price (used when expanding ladders).
    void set_price(types::Price price) noexcept { price_ = price; }

    /// Insert an order at the tail of the FIFO and update aggregates.
    void add(Order& order) noexcept;

    /// @return Pointer to the oldest resting order, or nullptr when empty.
    Order* top() noexcept;

    /// Remove the specified order from the FIFO and update aggregates.
    void remove(Order& order) noexcept;

    /// Apply a fill delta to the aggregate quantity.
    void on_fill(types::Quantity delta) noexcept;

private:
    types::Price price_{0};
    types::Quantity total_quantity_{0};
    IntrusiveFifo<OrderNode> orders_{};
};

} // namespace ob
