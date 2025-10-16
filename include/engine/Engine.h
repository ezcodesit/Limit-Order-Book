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

/**
 * @brief Command submitted by the CLI layer into the per-symbol engine.
 */
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

/**
 * @brief Per-symbol application managing ingress, matching, and logging threads.
 *
 * Incoming text commands are converted into @ref Command instances, queued in a
 * single-producer/single-consumer ring buffer, and processed deterministically
 * by a dedicated worker thread invoking @ref ob::OrderBook.
 */
class EngineApp {
public:
    /**
     * @brief Create a new engine instance for @p symbol.
     *
     * @param symbol Instrument identifier (used for log labelling).
     * @param min_price Initial minimum price supported by the book.
     * @param max_price Initial maximum price supported by the book.
     * @param pool_capacity Maximum number of live orders allocated from the pool.
     */
    explicit EngineApp(std::string symbol,
                       ob::types::Price min_price = 0,
                       ob::types::Price max_price = 1'000'000,
                       std::size_t      pool_capacity = 1'000'000);
    ~EngineApp();

    /**
     * @brief Submit a command for processing.
     *
     * Performs synchronous validation (ID mapping, duplicate checks) and enqueues
     * the command for the worker thread. Returns false if validation fails.
     */
    bool submit(Command cmd);

private:
    /// Worker thread body: drains ingress queue and forwards to the order book.
    void process();
    /// Logger thread body: flushes trade strings to stdout.
    void run_logger();
    /// Trade sink invoked by the order book (on the worker thread).
    void on_trade(const ob::Trade& trade);
    /// Static adapter passed to @ref ob::OrderBook so it can invoke @ref on_trade.
    static void trade_sink(const ob::Trade& trade, void* ctx);
    /// Map a client-supplied ID to an internal numeric identifier.
    ob::types::OrderId assign_order_id(const std::string& client_id);
    /// Lookup helper returning an internal ID when one exists.
    std::optional<ob::types::OrderId> find_order_id(const std::string& client_id) const;
    /// Resolve an internal ID back to the original client string.
    const std::string& to_client_id(ob::types::OrderId internal) const;

    std::atomic<bool> running_{true};
    std::string                 symbol_;
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
