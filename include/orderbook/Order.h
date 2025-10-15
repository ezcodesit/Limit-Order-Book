#pragma once

#ifdef USE_BOOST_INTRUSIVE
#include <boost/intrusive/list_hook.hpp>
#endif

#include "orderbook/Types.h"

#include <optional>

namespace ob {

struct Order;

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

struct Order {
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
