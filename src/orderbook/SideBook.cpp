#include "orderbook/SideBook.h"

namespace ob {

SideBook::SideBook(types::Side side)
    : side_(side)
    , ladder_(side_ == types::Side::Buy
                  ? std::function<bool(types::Price, types::Price)>(std::greater<types::Price>{})
                  : std::function<bool(types::Price, types::Price)>(std::less<types::Price>{})) {}

void SideBook::add(Order& order) {
    auto& level = price_map_.try_emplace(order.price, order.price).first->second;
    level.add(order);
    ladder_.insert(order.price);
}

void SideBook::remove(Order& order) {
    auto it = price_map_.find(order.price);
    if (it == price_map_.end()) return;
    it->second.remove(order);
    if (it->second.empty()) {
        price_map_.erase(it);
        ladder_.erase(order.price);
    }
}

Order* SideBook::best() {
    if (ladder_.empty()) return nullptr;
    auto price = *ladder_.begin();
    auto it = price_map_.find(price);
    if (it == price_map_.end()) return nullptr;
    return it->second.top();
}

void SideBook::on_fill(Order& order, types::Quantity delta) {
    auto it = price_map_.find(order.price);
    if (it == price_map_.end()) return;
    it->second.on_fill(delta);
}

} // namespace ob
