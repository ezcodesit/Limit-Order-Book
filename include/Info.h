// File: include/Info.h
#pragma once
#include <list>
#include "Order.h"

/**
 * Quick-lookup info for any resting GFD order:
 */
struct Info {
    std::list<Order>::iterator it;  // Position in the price-level list
    Price                     price; // Price level
    Side                      side;  // Which book it resides in
};