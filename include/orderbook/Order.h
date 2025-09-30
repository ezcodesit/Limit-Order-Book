#pragma once

#include "orderbook/Types.h"

#include <optional>

namespace ob {

struct Order;

struct alignas(64) OrderNode {
    Order*           order{nullptr};
    OrderNode*       next{nullptr};
    OrderNode*       prev{nullptr};
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
