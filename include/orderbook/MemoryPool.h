#pragma once

#include <cstddef>
#include <cstdint>
#include <new>
#include <type_traits>
#include <vector>
#include <utility>

namespace ob {

// Simple fixed-size memory pool for latency-sensitive allocations.
template <typename T>
class alignas(64) MemoryPool {
public:
    explicit MemoryPool(std::size_t capacity)
        : capacity_(capacity), storage_(capacity) {
        free_list_.reserve(capacity_);
        for (std::size_t i = 0; i < capacity_; ++i) {
            free_list_.push_back(reinterpret_cast<T*>(&storage_[i]));
        }
    }

    template <typename... Args>
    T* create(Args&&... args) {
        if (free_list_.empty()) return nullptr;
        T* slot = free_list_.back();
        free_list_.pop_back();
        return new (slot) T(std::forward<Args>(args)...);
    }

    void destroy(T* ptr) noexcept {
        if (!ptr) return;
        ptr->~T();
        free_list_.push_back(ptr);
    }

    std::size_t capacity() const noexcept { return capacity_; }
    std::size_t available() const noexcept { return free_list_.size(); }

private:
    std::size_t capacity_{};
    using slot_type = std::aligned_storage_t<sizeof(T), alignof(T)>;
    std::vector<slot_type> storage_{};
    std::vector<T*> free_list_{};
};

} // namespace ob
