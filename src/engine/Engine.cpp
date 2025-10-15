#include "engine/Engine.h"
#include "orderbook/Order.h"

#include <cstdio>

namespace engine {

EngineApp::EngineApp(std::string symbol,
                     ob::types::Price min_price,
                     ob::types::Price max_price,
                     std::size_t pool_capacity)
    : symbol_(std::move(symbol))
    , book_(min_price, max_price, pool_capacity)
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

bool EngineApp::submit(Command cmd) {
    switch (cmd.type) {
        case Command::Type::Buy:
        case Command::Type::Sell: {
            cmd.internal_id = assign_order_id(cmd.id);
            if (book_.has_order(cmd.internal_id)) {
                return false;
            }
            break;
        }
        case Command::Type::Cancel: {
            auto internal = find_order_id(cmd.id);
            if (!internal || !book_.has_order(*internal)) return false;
            cmd.internal_id = *internal;
            break;
        }
        case Command::Type::Modify: {
            auto internal = find_order_id(cmd.id);
            if (!internal || !book_.has_order(*internal)) return false;
            cmd.internal_id = *internal;
            break;
        }
        case Command::Type::Print:
            break;
    }

    while (!ingress_.push(cmd)) {
        std::this_thread::yield();
    }
    return true;
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
            case Command::Type::Modify: {
                const ob::Order* existing = book_.find(cmd->internal_id);
                if (!existing) break;
                auto tif = existing->tif;
                std::optional<ob::types::Quantity> min_qty = cmd->min_qty;
                if (!min_qty && existing->has_min_qty) {
                    min_qty = existing->min_qty;
                }
                book_.modify(cmd->internal_id,
                             cmd->side,
                             cmd->price,
                             cmd->qty,
                             tif,
                             min_qty);
                break;
            }
            case Command::Type::Print:
                std::cout << "Symbol: " << symbol_ << '\n';
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
    const std::string& resting = to_client_id(trade.resting_id);
    const std::string& incoming = to_client_id(trade.incoming_id);
    char buffer[160];
    int written = std::snprintf(buffer, sizeof(buffer),
                                "%s TRADE %s %lld %lld %s %lld %lld",
                                symbol_.c_str(),
                                resting.c_str(),
                                static_cast<long long>(trade.resting_px),
                                static_cast<long long>(trade.traded_qty),
                                incoming.c_str(),
                                static_cast<long long>(trade.incoming_px),
                                static_cast<long long>(trade.traded_qty));
    if (written <= 0) return;
    std::string line(buffer, static_cast<std::size_t>(written));
    while (!log_queue_.push(line)) {
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
