#include "engine/Engine.h"

#include <cstdio>
#include <sstream>

namespace engine {

EngineApp::EngineApp()
    : book_(/*min_price=*/0, /*max_price=*/1'000'000, /*pool_capacity=*/1'000'000)
    , ingress_(2048)
    , worker_([this] { process(); })
    , log_queue_(2048)
    , log_thread_([this] { run_logger(); })
{
    book_.set_trade_sink(&EngineApp::trade_sink, this);
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
            std::string tif_text;
            std::string client_id;
            ob::types::Price price;
            ob::types::Quantity qty;
            if (!(iss >> tif_text >> price >> qty >> client_id)) continue;

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
            cmd.id = client_id;
            cmd.internal_id = assign_order_id(client_id);
            cmd.price = price;
            cmd.qty = qty;
            cmd.side = (verb == "BUY") ? ob::types::Side::Buy : ob::types::Side::Sell;
            cmd.tif = tif;
            cmd.min_qty = min_qty;

            while (!ingress_.push(std::move(cmd))) {
                std::this_thread::yield();
            }
        } else if (verb == "CANCEL") {
            std::string client_id;
            if (!(iss >> client_id)) continue;
            auto internal = find_order_id(client_id);
            if (!internal) continue;

            Command cmd;
            cmd.type = Command::Type::Cancel;
            cmd.id = client_id;
            cmd.internal_id = *internal;
            while (!ingress_.push(std::move(cmd))) {
                std::this_thread::yield();
            }
        } else if (verb == "MODIFY") {
            std::string client_id;
            std::string side_text;
            ob::types::Price price;
            ob::types::Quantity qty;
            if (!(iss >> client_id >> side_text >> price >> qty)) continue;

            auto internal = find_order_id(client_id);
            if (!internal) continue;

            Command cmd;
            cmd.type = Command::Type::Modify;
            cmd.id = client_id;
            cmd.internal_id = *internal;
            cmd.price = price;
            cmd.qty = qty;
            cmd.side = (side_text == "BUY") ? ob::types::Side::Buy : ob::types::Side::Sell;
            cmd.tif = ob::types::TimeInForce::GFD;

            std::string token;
            while (iss >> token) {
                if (token == "MIN") {
                    ob::types::Quantity q;
                    if (iss >> q) cmd.min_qty = q;
                }
            }

            while (!ingress_.push(std::move(cmd))) {
                std::this_thread::yield();
            }
        } else if (verb == "PRINT") {
            Command cmd;
            cmd.type = Command::Type::Print;
            while (!ingress_.push(std::move(cmd))) {
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
                book_.create_order(cmd->internal_id,
                                   cmd->price,
                                   cmd->qty,
                                   cmd->side,
                                   cmd->tif,
                                   cmd->min_qty);
                break;
            case Command::Type::Cancel:
                book_.cancel(cmd->internal_id);
                break;
            case Command::Type::Modify:
                book_.modify(cmd->internal_id, cmd->price, cmd->qty, cmd->min_qty);
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

void EngineApp::on_trade(const ob::Trade& trade) {
    char buffer[128];
    int written = std::snprintf(buffer, sizeof(buffer),
                                "TRADE %s %lld %lld %s %lld %lld",
                                to_client_id(trade.resting_id).c_str(),
                                static_cast<long long>(trade.resting_px),
                                static_cast<long long>(trade.traded_qty),
                                to_client_id(trade.incoming_id).c_str(),
                                static_cast<long long>(trade.incoming_px),
                                static_cast<long long>(trade.traded_qty));
    if (written <= 0) return;
    std::string line(buffer, static_cast<std::size_t>(written));
    while (!log_queue_.push(std::move(line))) {
        std::this_thread::yield();
    }
}

void EngineApp::trade_sink(const ob::Trade& trade, void* ctx) {
    static_cast<EngineApp*>(ctx)->on_trade(trade);
}

ob::types::OrderId EngineApp::assign_order_id(const std::string& client_id) {
    auto [it, inserted] = id_lookup_.try_emplace(client_id, next_internal_id_);
    if (inserted) {
        id_reverse_.push_back(client_id);
        return next_internal_id_++;
    }
    return it->second;
}

std::optional<ob::types::OrderId> EngineApp::find_order_id(const std::string& client_id) const {
    auto it = id_lookup_.find(client_id);
    if (it == id_lookup_.end()) return std::nullopt;
    return it->second;
}

const std::string& EngineApp::to_client_id(ob::types::OrderId internal) const {
    static const std::string unknown{"<unknown>"};
    if (internal < id_reverse_.size()) return id_reverse_[static_cast<std::size_t>(internal)];
    return unknown;
}

} // namespace engine
