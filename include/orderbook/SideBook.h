#pragma once

#include "orderbook/Order.h"
#include "orderbook/PriceLevel.h"
#include "orderbook/Types.h"

#include <optional>
#include <vector>

namespace ob {

/**
 * @brief Manages all resting orders for a single side of the market.
 *
 * Maintains a dense ladder of @ref PriceLevel instances indexed by integerised price.
 * The ladder can grow in either direction and tracks the current best level to allow
 * constant-time access to the top of book.
 */
class SideBook {
public:
    /**
     * @brief Construct a side book initialised to cover @p [min_price, max_price].
     *
     * @param side Which side of the market the book represents.
     * @param min_price Lowest price initially addressable.
     * @param max_price Highest price initially addressable.
     */
    SideBook(types::Side side, types::Price min_price, types::Price max_price);

    /// Insert an order into the appropriate price level, expanding the ladder if needed.
    void add(Order& order);

    /// Remove an order from the ladder if it is currently resting.
    void remove(Order& order);

    /// @return Pointer to the best order (highest bid or lowest ask), or nullptr when empty.
    Order* best();

    /// Apply a fill delta to an order and update aggregates for its price level.
   void on_fill(Order& order, types::Quantity delta);

    /// @return True when no active price levels remain.
    bool empty() const noexcept { return active_count_ == 0; }

    template <typename Fn>
    void for_each_level(Fn&& fn) const {
        for (std::size_t idx = 0; idx < levels_.size(); ++idx) {
            if (!active_[idx]) continue;
            fn(levels_[idx]);
        }
    }

    /**
     * @brief Aggregate the quantity available at or better than @p limit_price.
     *
     * Used to decide whether FOK or minimum-quantity orders can proceed.
     *
     * @param limit_price Price constraint supplied by the incoming order.
     * @param incoming_side Side of the incoming order (Buy or Sell).
     * @return Total quantity resting within the acceptable price window.
     */
    types::Quantity available_to(types::Price limit_price, types::Side incoming_side) const;

private:
    /// Ensure the ladder can address @p price, expanding if required.
    void ensure_price(types::Price price);
    std::size_t index_of(types::Price price) const noexcept { return static_cast<std::size_t>(price - min_price_); }
    types::Price price_at(std::size_t index) const noexcept { return static_cast<types::Price>(min_price_ + static_cast<types::Price>(index)); }
    void update_best_on_insert(std::size_t idx);
    void recompute_best();
    std::size_t next_active_after(std::size_t idx) const noexcept;
    std::size_t prev_active_before(std::size_t idx) const noexcept;

    types::Side side_;
    types::Price min_price_;
    types::Price max_price_;
    std::vector<PriceLevel> levels_;
    std::vector<bool>       active_;
    std::size_t             active_count_{0};
    std::optional<std::size_t> best_index_{};
};

} // namespace ob
