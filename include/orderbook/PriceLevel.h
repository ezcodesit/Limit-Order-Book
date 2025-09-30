#pragma once

#include "orderbook/IntrusiveList.h"
#include "orderbook/Order.h"
#include "orderbook/Types.h"

namespace ob {

class PriceLevel {
public:
    explicit PriceLevel(types::Price price = 0) : price_(price) {}

    types::Price price() const noexcept { return price_; }
    types::Quantity total() const noexcept { return total_quantity_; }
    bool empty() const noexcept { return orders_.empty(); }

    void add(Order& order) noexcept;
    Order* top() noexcept;
    void remove(Order& order) noexcept;
    void on_fill(types::Quantity delta) noexcept;

private:
    types::Price price_{0};
    types::Quantity total_quantity_{0};
    IntrusiveFifo<OrderNode> orders_{};
};

} // namespace ob
