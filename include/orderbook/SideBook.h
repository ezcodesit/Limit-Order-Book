#pragma once

#include "orderbook/Order.h"
#include "orderbook/PriceLevel.h"
#include "orderbook/Types.h"

#include <unordered_map>
#include <set>

namespace ob {

class SideBook {
public:
    explicit SideBook(types::Side side);

    void add(Order& order);
    void remove(Order& order);
    Order* best();
    void on_fill(Order& order, types::Quantity delta);
    bool empty() const noexcept { return price_map_.empty(); }

    template <typename Fn>
    void for_each_level(Fn&& fn) const {
        for (const auto& [price, level] : price_map_) {
            fn(level);
        }
    }

private:
    types::Side side_;
    std::unordered_map<types::Price, PriceLevel> price_map_;
    std::set<types::Price, std::function<bool(types::Price, types::Price)>> ladder_;
};

} // namespace ob
