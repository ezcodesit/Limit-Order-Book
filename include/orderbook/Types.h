#pragma once

#include <cstdint>

template <typename T>
constexpr T align_to_cacheline(T size) noexcept {
    constexpr T cache_line = 64;
    return ((size + cache_line - 1) / cache_line) * cache_line;
}

namespace ob::types {

using Price    = std::int64_t;   // integerised price ticks
using Quantity = std::int64_t;   // integral quantities
using OrderId  = std::uint64_t;  // internal order identifier

enum class Side : std::uint8_t { Buy, Sell };
enum class TimeInForce : std::uint8_t { GFD, IOC, FOK };

inline constexpr OrderId invalid_order_id = static_cast<OrderId>(-1);

} // namespace ob::types
