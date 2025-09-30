#pragma once

#include "orderbook/OrderBook.h"
#include "orderbook/SpscRingBuffer.h"

#include <atomic>
#include <iostream>
#include <optional>
#include <thread>
#include <string>
#include <unordered_map>
#include <vector>

namespace engine {

struct Command {
    enum class Type { Buy, Sell, Cancel, Modify, Print };

    Type type{Type::Print};
    std::string id;
    ob::types::OrderId internal_id{ob::types::invalid_order_id};
    ob::types::Price price{0};
    ob::types::Quantity qty{0};
    ob::types::Side side{ob::types::Side::Buy};
    ob::types::TimeInForce tif{ob::types::TimeInForce::GFD};
    std::optional<ob::types::Quantity> min_qty;
};

class EngineApp {
public:
    EngineApp();
    ~EngineApp();

    void run_cli();

private:
    void process();
    void run_logger();
    void on_trade(const ob::Trade& trade);
    static void trade_sink(const ob::Trade& trade, void* ctx);
    ob::types::OrderId assign_order_id(const std::string& client_id);
    std::optional<ob::types::OrderId> find_order_id(const std::string& client_id) const;
    const std::string& to_client_id(ob::types::OrderId internal) const;

    std::atomic<bool> running_{true};
    ob::OrderBook               book_;
    ob::SpscRingBuffer<Command> ingress_;
    std::thread                 worker_;
    ob::SpscRingBuffer<std::string> log_queue_;
    std::thread                 log_thread_;
    std::unordered_map<std::string, ob::types::OrderId> id_lookup_;
    std::vector<std::string>                         id_reverse_;
    ob::types::OrderId                               next_internal_id_{0};
};

} // namespace engine
