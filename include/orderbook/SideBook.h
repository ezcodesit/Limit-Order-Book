#pragma once

#include "orderbook/Order.h"
#include "orderbook/PriceLevel.h"
#include "orderbook/Types.h"

#include <optional>
#include <vector>

namespace ob {

class SideBook {
public:
    SideBook(types::Side side, types::Price min_price, types::Price max_price);

    void add(Order& order);
    void remove(Order& order);
    Order* best();
    void on_fill(Order& order, types::Quantity delta);
    bool empty() const noexcept { return active_count_ == 0; }

    template <typename Fn>
    void for_each_level(Fn&& fn) const {
        for (std::size_t idx = 0; idx < levels_.size(); ++idx) {
            if (!active_[idx]) continue;
            fn(levels_[idx]);
        }
    }

    types::Quantity available_to(types::Price limit_price, types::Side incoming_side) const;

private:
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
