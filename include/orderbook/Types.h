#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

template <typename T>
constexpr T align_to_cacheline(T size) noexcept {
    constexpr T cache_line = 64;
    return ((size + cache_line - 1) / cache_line) * cache_line;
}

namespace ob::types {
using Price   = std::int64_t;   // integerised price ticks
using Quantity = std::int64_t;  // integral quantities

enum class Side : std::uint8_t { Buy, Sell };
enum class TimeInForce : std::uint8_t { GFD, IOC, FOK };

struct OrderIdHash {
    std::size_t operator()(std::string_view id) const noexcept {
        std::size_t h = 0xcbf29ce484222325ULL;
        for (unsigned char c : id) {
            h ^= c;
            h *= 0x100000001b3ULL;
        }
        return h;
    }
};

} // namespace ob::types
