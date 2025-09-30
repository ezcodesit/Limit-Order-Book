#pragma once

#include "orderbook/Types.h"

#include <atomic>
#include <optional>

namespace ob {

struct Order;

struct alignas(64) OrderNode {
    Order*           order{nullptr};
    OrderNode*       next{nullptr};
    OrderNode*       prev{nullptr};
};

struct Order {
    Order(std::string id_,
          types::Price price_,
          types::Quantity qty_,
          types::Side side_,
          types::TimeInForce tif_,
          std::optional<types::Quantity> min_qty_ = std::nullopt)
        : id(std::move(id_))
        , price(price_)
        , quantity(qty_)
        , side(side_)
        , tif(tif_)
        , min_qty(min_qty_) {}

    std::string      id;
    types::Price     price{0};
    types::Quantity  quantity{0};
    types::Side      side{types::Side::Buy};
    types::TimeInForce tif{types::TimeInForce::GFD};
    std::optional<types::Quantity> min_qty;

    OrderNode node{};
};

} // namespace ob
