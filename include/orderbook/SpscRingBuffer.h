#pragma once

#include <atomic>
#include <cstddef>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace ob {

/**
 * @brief Bounded single-producer single-consumer ring buffer.
 *
 * @tparam T Payload type stored in the queue.
 *
 * The implementation relies solely on atomic operations, making it suitable
 * for cross-thread hand-offs with minimal overhead.
 */
template <typename T>
class alignas(64) SpscRingBuffer {
public:
    /**
     * @brief Construct a ring buffer with a power-of-two capacity.
     * @param capacity Number of elements the buffer can hold.
     * @throw std::invalid_argument When @p capacity is not a power of two or < 2.
     */
    explicit SpscRingBuffer(std::size_t capacity)
        : capacity_(capacity)
        , mask_(capacity - 1)
        , buffer_(capacity) {
        if (capacity < 2) {
            throw std::invalid_argument("capacity must be at least 2 and power of two");
        }
        if ((capacity_ & mask_) != 0) {
            throw std::invalid_argument("capacity must be power of two");
        }
    }

    /**
     * @brief Enqueue a copy of @p item.
     * @return @c true when the item was enqueued, @c false when the buffer was full.
     */
    bool push(const T& item) noexcept {
        return emplace(item);
    }

    /**
     * @brief Enqueue a moved instance of @p item.
     * @return @c true when the item was enqueued, @c false when the buffer was full.
     */
    bool push(T&& item) noexcept {
        return emplace(std::move(item));
    }

private:
    template <typename U>
    bool emplace(U&& item) noexcept {
        auto head = head_.load(std::memory_order_relaxed);
        auto next = (head + 1) & mask_;
        if (next == tail_.load(std::memory_order_acquire)) {
            return false; // full
        }
        buffer_[head] = std::forward<U>(item);
        head_.store(next, std::memory_order_release);
        return true;
    }

public:

    /**
     * @brief Pop the next item from the queue.
     * @return A populated optional when an item was available; @c std::nullopt otherwise.
     */
    std::optional<T> pop() noexcept {
        auto tail = tail_.load(std::memory_order_relaxed);
        if (tail == head_.load(std::memory_order_acquire)) {
            return std::nullopt; // empty
        }
        T item = std::move(buffer_[tail]);
        tail_.store((tail + 1) & mask_, std::memory_order_release);
        return item;
    }

private:
    std::size_t capacity_;
    std::size_t mask_;
    std::vector<T> buffer_;
    std::atomic<std::size_t> head_{0};
    std::atomic<std::size_t> tail_{0};
};

} // namespace ob
