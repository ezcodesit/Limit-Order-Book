#include "orderbook/PriceLevel.h"

namespace ob {

void PriceLevel::add(Order& order) noexcept {
    total_quantity_ += order.quantity;
    orders_.push_back(&order.node);
    order.node.order = &order;
    order.resting = true;
}

Order* PriceLevel::top() noexcept {
    auto* node = orders_.front();
    return node ? node->order : nullptr;
}

void PriceLevel::remove(Order& order) noexcept {
    total_quantity_ -= order.quantity;
    if (total_quantity_ < 0) total_quantity_ = 0;
    orders_.erase(&order.node);
    order.resting = false;
    order.node.order = nullptr;
}

void PriceLevel::on_fill(types::Quantity delta) noexcept {
    total_quantity_ -= delta;
    if (total_quantity_ < 0) total_quantity_ = 0;
}

} // namespace ob
