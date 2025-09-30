// File: include/Order.h
#pragma once
#include <string>
#include <cstdint>

using Price = int64_t;
using Quantity = int64_t;

enum class Side { Buy, Sell };
enum class TIF  { GFD, IOC };

/**
 * Represents an order being matched or resting in the book.
 */
struct Order {
    std::string id;   // Unique order identifier
    Price       price; // Limit price
    Quantity    qty;   // Remaining quantity to match or rest
    Side        side;  // Buy or Sell
    TIF         tif;   // Time-In-Force: GFD or IOC
};