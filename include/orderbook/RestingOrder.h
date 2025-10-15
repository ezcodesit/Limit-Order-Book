#pragma once

#include <list>

#include "orderbook/Order.h"
#include "orderbook/Types.h"

namespace ob {

// Quick-lookup info for any resting GFD order.
struct RestingOrder {
    std::list<Order>::iterator it;    // Position in the price-level list
    types::Price               price; // Price level
    types::Side                side;  // Which book it resides in
};

} // namespace ob
