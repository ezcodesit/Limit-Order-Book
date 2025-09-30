#include "engine/Engine.h"

#include <sstream>

namespace engine {

EngineApp::EngineApp()
    : book_(1'000)
    , ingress_(1024)
    , worker_([this] { process(); })
    , log_queue_(1024)
    , log_thread_([this] { run_logger(); })
{
    book_.set_trade_callback([this](const ob::Trade& trade) {
        std::ostringstream oss;
        oss << "TRADE " << trade.resting_id << ' ' << trade.resting_px
            << ' ' << trade.traded_qty << ' ' << trade.incoming_id
            << ' ' << trade.incoming_px << ' ' << trade.traded_qty;
        std::string line = oss.str();
        while (!log_queue_.push(line)) {
            std::this_thread::yield();
        }
    });
}

EngineApp::~EngineApp() {
    running_.store(false, std::memory_order_release);
    if (worker_.joinable()) worker_.join();
    if (log_thread_.joinable()) log_thread_.join();
}

void EngineApp::run_cli() {
    std::string line;
    while (std::getline(std::cin, line)) {
        std::istringstream iss(line);
        std::string verb;
        iss >> verb;
        if (verb == "BUY" || verb == "SELL") {
            std::string tif_text, id;
            ob::types::Price price;
            ob::types::Quantity qty;
            iss >> tif_text >> price >> qty >> id;
            auto tif = ob::types::TimeInForce::GFD;
            if (tif_text == "IOC") tif = ob::types::TimeInForce::IOC;
            else if (tif_text == "FOK") tif = ob::types::TimeInForce::FOK;
            std::optional<ob::types::Quantity> min_qty;
            std::string token;
            while (iss >> token) {
                if (token == "MIN") {
                    ob::types::Quantity q;
                    if (iss >> q) min_qty = q;
                }
            }
            Command cmd;
            cmd.type = verb == "BUY" ? Command::Type::Buy : Command::Type::Sell;
            cmd.id = id;
            cmd.price = price;
            cmd.qty = qty;
            cmd.side = verb == "BUY" ? ob::types::Side::Buy : ob::types::Side::Sell;
            cmd.tif = tif;
            cmd.min_qty = min_qty;
            while (!ingress_.push(cmd)) {
                std::this_thread::yield();
            }
        } else if (verb == "CANCEL") {
            Command cmd;
            cmd.type = Command::Type::Cancel;
            iss >> cmd.id;
            while (!ingress_.push(cmd)) {
                std::this_thread::yield();
            }
        } else if (verb == "MODIFY") {
            Command cmd;
            cmd.type = Command::Type::Modify;
            std::string side_text;
            iss >> cmd.id >> side_text >> cmd.price >> cmd.qty;
            cmd.side = (side_text == "BUY") ? ob::types::Side::Buy : ob::types::Side::Sell;
            cmd.tif = ob::types::TimeInForce::GFD;
            std::string token;
            while (iss >> token) {
                if (token == "MIN") {
                    ob::types::Quantity q;
                    if (iss >> q) cmd.min_qty = q;
                }
            }
            while (!ingress_.push(cmd)) {
                std::this_thread::yield();
            }
        } else if (verb == "PRINT") {
            Command cmd;
            cmd.type = Command::Type::Print;
            while (!ingress_.push(cmd)) {
                std::this_thread::yield();
            }
        }
    }
}

void EngineApp::process() {
    while (running_.load(std::memory_order_acquire)) {
        auto cmd = ingress_.pop();
        if (!cmd) {
            std::this_thread::yield();
            continue;
        }
        switch (cmd->type) {
            case Command::Type::Buy:
            case Command::Type::Sell:
                book_.create_order(cmd->id,
                                   cmd->price,
                                   cmd->qty,
                                   cmd->side,
                                   cmd->tif,
                                   cmd->min_qty);
                break;
            case Command::Type::Cancel:
                book_.cancel(cmd->id);
                break;
            case Command::Type::Modify:
                book_.modify(cmd->id, cmd->price, cmd->qty, cmd->min_qty);
                break;
            case Command::Type::Print:
                book_.snapshot(std::cout);
                break;
        }
    }
}

void EngineApp::run_logger() {
    for (;;) {
        auto msg = log_queue_.pop();
        if (msg) {
            std::cout << *msg << '\n';
            continue;
        }
        if (!running_.load(std::memory_order_acquire)) {
            msg = log_queue_.pop();
            if (!msg) break;
            std::cout << *msg << '\n';
            continue;
        }
        std::this_thread::yield();
    }
}

} // namespace engine
