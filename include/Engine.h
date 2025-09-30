// File: include/Engine.h
#pragma once
#include <algorithm>
#include <functional>
#include <iostream>
#include <iterator>
#include <list>
#include <map>
#include <string>
#include <unordered_map>
#include "Order.h"
#include "Info.h"

/**
 * Engine handles order matching, rest, cancel, modify, and print operations.
 */
class Engine {
public:
    Engine();

    void onNewOrder(const Order& in);
    void onCancel(const std::string& oid);
    void onModify(const std::string& oid, Side newSide, Price newPrice, Quantity newQty);
    void onPrint() const;
    bool hasId(const std::string& oid) const noexcept;

private:
    template<typename OppBook, typename Pred>
    Quantity matchLoop(Order in, OppBook& opp, Pred crossPrice);

    template<typename Book>
    void insertGFD(const Order& in, Book& book);

    std::map<Price, std::list<Order>, std::greater<Price>> buyBook;
    std::map<Price, std::list<Order>>                        sellBook;
    std::unordered_map<std::string, Info>                    idMap;
};

// Template implementations must be visible in header

template<typename OppBook, typename Pred>
Quantity Engine::matchLoop(Order in, OppBook& opp, Pred crossPrice) {
    Quantity rem = in.qty;
    while (rem > 0 && !opp.empty()) {
        auto pit = opp.begin();
        Price restingPrice = pit->first;
        if (!crossPrice(restingPrice)) break;

        auto& lst = pit->second;
        auto it  = lst.begin();
        Quantity tq = std::min(rem, it->qty);

        std::cout << "TRADE "
                  << it->id << ' ' << it->price << ' ' << tq << ' '
                  << in.id  << ' ' << in.price << ' ' << tq << '\n';

        it->qty -= tq;
        rem    -= tq;

        if (it->qty == 0) {
            idMap.erase(it->id);
            lst.erase(it);
            if (lst.empty()) opp.erase(pit);
        }
    }
    return rem;
}

template<typename Book>
void Engine::insertGFD(const Order& in, Book& book) {
    auto price = in.price;
    auto& lst = book[price];
    lst.push_back(in);
    auto lit = std::prev(lst.end());
    idMap[lit->id] = Info{ lit, price, in.side };
}
