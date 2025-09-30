// File: src/main.cpp
#include <iostream>
#include <string>
#include "Order.h"
#include "Engine.h"

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    Engine engine;
    std::string cmd;

    while (std::cin >> cmd) {
        if (cmd == "BUY" || cmd == "SELL") {
            std::string type, oid;
            Price price;
            Quantity qty;
            std::cin >> type >> price >> qty >> oid;
            if (!oid.empty() && price > 0 && qty > 0 &&
                (type == "GFD" || type == "IOC") &&
                !engine.hasId(oid))
            {
                Side side = (cmd == "BUY") ? Side::Buy : Side::Sell;
                TIF tif   = (type == "GFD") ? TIF::GFD : TIF::IOC;
                engine.onNewOrder(Order{oid, price, qty, side, tif});
            }
        }
        else if (cmd == "CANCEL") {
            std::string oid; std::cin >> oid;
            engine.onCancel(oid);
        }
        else if (cmd == "MODIFY") {
            std::string oid, sideStr;
            Price price;
            Quantity qty;
            std::cin >> oid >> sideStr >> price >> qty;
            Side side = (sideStr == "BUY") ? Side::Buy : Side::Sell;
            engine.onModify(oid, side, price, qty);
        }
        else if (cmd == "PRINT") {
            engine.onPrint();
        } else {
            std::string trash; std::getline(std::cin, trash);
        }
    }
    return 0;
}