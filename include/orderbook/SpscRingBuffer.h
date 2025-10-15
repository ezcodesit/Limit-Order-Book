#pragma once

#include <atomic>
#include <cstddef>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace ob {

// Bounded single-producer single-consumer ring buffer.
template <typename T>
class alignas(64) SpscRingBuffer {
public:
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

    bool push(const T& item) noexcept {
        return emplace(item);
    }

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
