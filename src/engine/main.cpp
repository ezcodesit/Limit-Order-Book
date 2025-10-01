#include "engine/Engine.h"

#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>

using engine::Command;

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    std::unordered_map<std::string, std::unique_ptr<engine::EngineApp>> engines;

    std::string line;
    while (std::getline(std::cin, line)) {
        if (line.empty()) continue;
        std::istringstream iss(line);
        std::string symbol;
        std::string verb;
        if (!(iss >> symbol >> verb)) continue;

        auto& engine_ptr = engines[symbol];
        if (!engine_ptr) {
            engine_ptr = std::make_unique<engine::EngineApp>(symbol);
        }
        auto& engine = *engine_ptr;

        if (verb == "BUY" || verb == "SELL") {
            std::string tif_text;
            ob::types::Price price;
            ob::types::Quantity qty;
            std::string client_id;
            if (!(iss >> tif_text >> price >> qty >> client_id)) continue;

            auto tif = ob::types::TimeInForce::GFD;
            if (tif_text == "IOC") tif = ob::types::TimeInForce::IOC;
            else if (tif_text == "FOK") tif = ob::types::TimeInForce::FOK;

            std::optional<ob::types::Quantity> min_qty;
            std::string token;
            while (iss >> token) {
                if (token == "MIN") {
                    ob::types::Quantity m;
                    if (iss >> m) min_qty = m;
                }
            }

            Command cmd;
            cmd.type = (verb == "BUY") ? Command::Type::Buy : Command::Type::Sell;
            cmd.id = client_id;
            cmd.price = price;
            cmd.qty = qty;
            cmd.side = (verb == "BUY") ? ob::types::Side::Buy : ob::types::Side::Sell;
            cmd.tif = tif;
            cmd.min_qty = min_qty;
            engine.submit(std::move(cmd));
        } else if (verb == "CANCEL") {
            std::string client_id;
            if (!(iss >> client_id)) continue;
            Command cmd;
            cmd.type = Command::Type::Cancel;
            cmd.id = client_id;
            engine.submit(std::move(cmd));
        } else if (verb == "MODIFY") {
            std::string client_id;
            std::string side_text;
            ob::types::Price price;
            ob::types::Quantity qty;
            if (!(iss >> client_id >> side_text >> price >> qty)) continue;
            Command cmd;
            cmd.type = Command::Type::Modify;
            cmd.id = client_id;
            cmd.price = price;
            cmd.qty = qty;
            cmd.side = (side_text == "BUY") ? ob::types::Side::Buy : ob::types::Side::Sell;
            std::string token;
            while (iss >> token) {
                if (token == "MIN") {
                    ob::types::Quantity m;
                    if (iss >> m) cmd.min_qty = m;
                }
            }
            engine.submit(std::move(cmd));
        } else if (verb == "PRINT") {
            Command cmd;
            cmd.type = Command::Type::Print;
            engine.submit(std::move(cmd));
        }
    }
    return 0;
}
