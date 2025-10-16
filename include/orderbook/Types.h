#pragma once

#include <cstdint>

/**
 * @brief Align a size to the CPU cache line boundary.
 *
 * @tparam T Integral size type.
 * @param size Number of bytes to align.
 * @return The next multiple of the cache line size greater than or equal to @p size.
 */
template <typename T>
constexpr T align_to_cacheline(T size) noexcept {
    constexpr T cache_line = 64;
    return ((size + cache_line - 1) / cache_line) * cache_line;
}

namespace ob::types {

/// Logical price expressed in integer ticks.
using Price = std::int64_t;

/// Logical quantity expressed in whole units.
using Quantity = std::int64_t;

/// Compact internal identifier assigned by the engine.
using OrderId = std::uint64_t;

/// Order side selection.
enum class Side : std::uint8_t { Buy, Sell };

/// Time-in-force semantics attached to an order.
enum class TimeInForce : std::uint8_t { GFD, IOC, FOK };

/// Sentinel used when an order identifier is invalid or absent.
inline constexpr OrderId invalid_order_id = static_cast<OrderId>(-1);

} // namespace ob::types
