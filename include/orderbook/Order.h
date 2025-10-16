#pragma once

#ifdef USE_BOOST_INTRUSIVE
#include <boost/intrusive/list_hook.hpp>
#endif

#include "orderbook/Types.h"

#include <optional>

namespace ob {

struct Order;

/**
 * @brief Intrusive node embedded inside an @ref Order for price-level queues.
 */
struct alignas(64) OrderNode
#ifdef USE_BOOST_INTRUSIVE
    : boost::intrusive::list_base_hook<boost::intrusive::link_mode<boost::intrusive::normal_link>>
#endif
{
    Order* order{nullptr};
#ifndef USE_BOOST_INTRUSIVE
    OrderNode* next{nullptr};
    OrderNode* prev{nullptr};
#endif
};

/**
 * @brief Representation of a single client order.
 *
 * Instances are allocated from a fixed pool and never move in memory, allowing the
 * rest of the book to hold raw pointers to them. Each order embeds an @ref OrderNode
 * so it can participate in intrusive price-level queues without extra allocations.
 */
struct Order {
    /**
     * @brief Construct a logical order payload.
     *
     * @param id_         Internal numeric identifier (assigned by the caller).
     * @param price_      Limit price expressed in ticks.
     * @param qty_        Remaining quantity.
     * @param side_       Buy or sell intent.
     * @param tif_        Time-in-force semantics.
     * @param min_qty_    Optional minimum acceptable fill quantity.
     */
    Order(types::OrderId id_,
          types::Price price_,
          types::Quantity qty_,
          types::Side side_,
          types::TimeInForce tif_,
          std::optional<types::Quantity> min_qty_ = std::nullopt)
        : id(id_)
        , price(price_)
        , quantity(qty_)
        , side(side_)
        , tif(tif_)
    {
        if (min_qty_) {
            min_qty = *min_qty_;
            has_min_qty = true;
        }
    }

    types::OrderId     id{types::invalid_order_id};
    types::Price       price{0};
    types::Quantity    quantity{0};
    types::Side        side{types::Side::Buy};
    types::TimeInForce tif{types::TimeInForce::GFD};
    types::Quantity    min_qty{0};
    bool               has_min_qty{false};

    OrderNode node{};
    bool      resting{false};
};

} // namespace ob
