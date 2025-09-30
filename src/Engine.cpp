// File: src/Engine.cpp
#include "Engine.h"
#include <iostream>

Engine::Engine() {
    idMap.reserve(1 << 20);
}

void Engine::onNewOrder(const Order& in) {
    Order order = in;
    if (order.side == Side::Buy) {
        Quantity rem = matchLoop(order, sellBook, [&](Price p){ return order.price >= p; });
        if (order.tif == TIF::GFD && rem > 0) {
            order.qty = rem;
            insertGFD(order, buyBook);
        }
    } else {
        Quantity rem = matchLoop(order, buyBook, [&](Price p){ return order.price <= p; });
        if (order.tif == TIF::GFD && rem > 0) {
            order.qty = rem;
            insertGFD(order, sellBook);
        }
    }
}

void Engine::onCancel(const std::string& oid) {
    auto it = idMap.find(oid);
    if (it == idMap.end()) return;
    auto info = it->second;
    if (info.side == Side::Buy) {
        auto lvl = buyBook.find(info.price);
        lvl->second.erase(info.it);
        if (lvl->second.empty()) buyBook.erase(lvl);
    } else {
        auto lvl = sellBook.find(info.price);
        lvl->second.erase(info.it);
        if (lvl->second.empty()) sellBook.erase(lvl);
    }
    idMap.erase(it);
}

void Engine::onModify(const std::string& oid, Side newSide, Price newPrice, Quantity newQty) {
    auto it = idMap.find(oid);
    if (it == idMap.end()) return;

    if (newQty <= 0 || newPrice <= 0) {
        onCancel(oid);
        return;
    }

    Info info = it->second;
    Order updated = *info.it;
    updated.side  = newSide;
    updated.price = newPrice;
    updated.qty   = newQty;

    onCancel(oid);
    onNewOrder(updated);
}

void Engine::onPrint() const {
    std::cout << "SELL:\n";
    for (const auto& lvl : sellBook) {
        Quantity sum = 0;
        for (const auto& o : lvl.second) sum += o.qty;
        if (sum) std::cout << lvl.first << ' ' << sum << "\n";
    }
    std::cout << "BUY:\n";
    for (const auto& lvl : buyBook) {
        Quantity sum = 0;
        for (const auto& o : lvl.second) sum += o.qty;
        if (sum) std::cout << lvl.first << ' ' << sum << "\n";
    }
}

bool Engine::hasId(const std::string& oid) const noexcept {
    return idMap.find(oid) != idMap.end();
}
