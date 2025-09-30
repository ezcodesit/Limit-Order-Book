#include "orderbook/SideBook.h"

#include <algorithm>

namespace ob {

SideBook::SideBook(types::Side side, types::Price min_price, types::Price max_price)
    : side_(side)
    , min_price_(min_price)
    , max_price_(max_price) {
    if (min_price_ > max_price_) std::swap(min_price_, max_price_);
    const auto span = static_cast<std::size_t>(max_price_ - min_price_ + 1);
    levels_.reserve(span);
    active_.reserve(span);
    for (std::size_t i = 0; i < span; ++i) {
        types::Price px = static_cast<types::Price>(min_price_ + static_cast<types::Price>(i));
        levels_.emplace_back(px);
        active_.push_back(false);
    }
}

void SideBook::ensure_price(types::Price price) {
    if (levels_.empty()) {
        min_price_ = max_price_ = price;
        levels_.emplace_back(price);
        active_.push_back(false);
        return;
    }

    if (price < min_price_) {
        const auto add = static_cast<std::size_t>(min_price_ - price);
        levels_.insert(levels_.begin(), add, PriceLevel{});
        active_.insert(active_.begin(), add, false);
        for (std::size_t i = 0; i < levels_.size(); ++i) {
            types::Price px = static_cast<types::Price>(price + static_cast<types::Price>(i));
            levels_[i].set_price(px);
        }
        min_price_ = price;
        if (best_index_) *best_index_ += add;
        return;
    }

    if (price > max_price_) {
        const auto add = static_cast<std::size_t>(price - max_price_);
        auto current_size = levels_.size();
        levels_.resize(current_size + add);
        active_.resize(current_size + add, false);
        for (std::size_t i = 0; i < add; ++i) {
            types::Price px = static_cast<types::Price>(max_price_ + static_cast<types::Price>(i + 1));
            levels_[current_size + i].set_price(px);
        }
        max_price_ = price;
    }
}

void SideBook::update_best_on_insert(std::size_t idx) {
    if (!best_index_) {
        best_index_ = idx;
        return;
    }
    const types::Price current_best = price_at(*best_index_);
    const types::Price candidate    = price_at(idx);
    if (side_ == types::Side::Buy) {
        if (candidate > current_best) best_index_ = idx;
    } else {
        if (candidate < current_best) best_index_ = idx;
    }
}

void SideBook::recompute_best() {
    best_index_.reset();
    if (active_count_ == 0) return;

    if (side_ == types::Side::Buy) {
        for (std::size_t idx = levels_.size(); idx-- > 0;) {
            if (!active_[idx]) continue;
            best_index_ = idx;
            return;
        }
    } else {
        for (std::size_t idx = 0; idx < levels_.size(); ++idx) {
            if (!active_[idx]) continue;
            best_index_ = idx;
            return;
        }
    }
}

std::size_t SideBook::next_active_after(std::size_t idx) const noexcept {
    for (std::size_t i = idx + 1; i < levels_.size(); ++i) {
        if (active_[i]) return i;
    }
    return levels_.size();
}

std::size_t SideBook::prev_active_before(std::size_t idx) const noexcept {
    for (std::size_t i = idx; i-- > 0;) {
        if (active_[i]) return i;
    }
    return levels_.size();
}

void SideBook::add(Order& order) {
    ensure_price(order.price);
    const auto idx = index_of(order.price);
    auto& level = levels_[idx];
    if (!active_[idx]) {
        level.set_price(order.price);
        active_[idx] = true;
        ++active_count_;
        update_best_on_insert(idx);
    }
    level.add(order);
}

void SideBook::remove(Order& order) {
    if (order.price < min_price_ || order.price > max_price_) return;
    const auto idx = index_of(order.price);
    auto& level = levels_[idx];
    level.remove(order);
    if (level.empty()) {
        if (active_[idx]) {
            active_[idx] = false;
            if (active_count_ > 0) --active_count_;
            if (best_index_ && *best_index_ == idx) {
                recompute_best();
            }
        }
    }
}

Order* SideBook::best() {
    if (!best_index_) {
        recompute_best();
        if (!best_index_) return nullptr;
    }

    auto& level = levels_[*best_index_];
    auto* top_order = level.top();
    if (top_order) return top_order;

    // best level is empty due to partial fills; mark inactive and retry
    if (active_[*best_index_]) {
        active_[*best_index_] = false;
        if (active_count_ > 0) --active_count_;
    }
    best_index_.reset();
    return best();
}

void SideBook::on_fill(Order& order, types::Quantity delta) {
    if (order.price < min_price_ || order.price > max_price_) return;
    const auto idx = index_of(order.price);
    auto& level = levels_[idx];
    level.on_fill(delta);
    if (level.total() == 0 && level.empty() && active_[idx]) {
        active_[idx] = false;
        if (active_count_ > 0) --active_count_;
        if (best_index_ && *best_index_ == idx) {
            recompute_best();
        }
    }
}

types::Quantity SideBook::available_to(types::Price limit_price, types::Side incoming_side) const {
    if (levels_.empty() || active_count_ == 0 || !best_index_) return 0;
    types::Quantity total = 0;
    if (incoming_side == types::Side::Buy) {
        auto idx = *best_index_;
        if (price_at(idx) > limit_price) return 0;
        while (idx < levels_.size() && price_at(idx) <= limit_price) {
            if (active_[idx]) total += levels_[idx].total();
            auto next = next_active_after(idx);
            if (next == levels_.size()) break;
            idx = next;
        }
    } else {
        auto idx = *best_index_;
        if (price_at(idx) < limit_price) return 0;
        while (idx < levels_.size() && price_at(idx) >= limit_price) {
            if (active_[idx]) total += levels_[idx].total();
            auto prev = prev_active_before(idx);
            if (prev == levels_.size()) break;
            idx = prev;
        }
    }
    return total;
}

} // namespace ob
