#pragma once

#include <cstddef>
#include <cstdint>
#include <new>
#include <type_traits>
#include <vector>
#include <utility>

namespace ob {

/**
 * @brief Fixed-size memory pool for latency-sensitive allocations.
 *
 * @tparam T Object type stored in the pool.
 *
 * The pool pre-allocates raw storage for @p capacity objects and serves allocations
 * via placement new. Destruction returns the slot to the free list without touching
 * the system allocator.
 */
template <typename T>
class alignas(64) MemoryPool {
public:
    /**
     * @brief Construct a pool with space for @p capacity objects.
     * @param capacity Maximum number of simultaneously live objects.
     */
    explicit MemoryPool(std::size_t capacity)
        : capacity_(capacity), storage_(capacity) {
        free_list_.reserve(capacity_);
        for (std::size_t i = 0; i < capacity_; ++i) {
            free_list_.push_back(reinterpret_cast<T*>(&storage_[i]));
        }
    }

    /**
     * @brief Allocate and construct an object in-place.
     *
     * @tparam Args Forwarded constructor argument types.
     * @param args Constructor arguments forwarded to @c T .
     * @return Pointer to the newly constructed object, or nullptr if the pool is exhausted.
     */
    template <typename... Args>
    T* create(Args&&... args) {
        if (free_list_.empty()) return nullptr;
        T* slot = free_list_.back();
        free_list_.pop_back();
        return new (slot) T(std::forward<Args>(args)...);
    }

    /**
     * @brief Destroy an object and return its storage to the pool.
     * @param ptr Pointer previously obtained from @ref create.
     */
    void destroy(T* ptr) noexcept {
        if (!ptr) return;
        ptr->~T();
        free_list_.push_back(ptr);
    }

    /// @return Maximum number of concurrently live objects.
    std::size_t capacity() const noexcept { return capacity_; }

    /// @return Number of free slots currently available.
   std::size_t available() const noexcept { return free_list_.size(); }

private:
    std::size_t capacity_{};
    using slot_type = std::aligned_storage_t<sizeof(T), alignof(T)>;
    std::vector<slot_type> storage_{};
    std::vector<T*> free_list_{};
};

} // namespace ob
