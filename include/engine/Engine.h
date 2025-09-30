#pragma once

#include "orderbook/OrderBook.h"
#include "orderbook/SpscRingBuffer.h"

#include <atomic>
#include <iostream>
#include <optional>
#include <thread>
#include <string>

namespace engine {

struct Command {
    enum class Type { Buy, Sell, Cancel, Modify, Print };

    Type type{Type::Print};
    std::string id;
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

    std::atomic<bool> running_{true};
    ob::OrderBook               book_;
    ob::SpscRingBuffer<Command> ingress_;
    std::thread                 worker_;
    ob::SpscRingBuffer<std::string> log_queue_;
    std::thread                 log_thread_;
};

} // namespace engine
