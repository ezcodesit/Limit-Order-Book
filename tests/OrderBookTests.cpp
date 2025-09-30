#include <gtest/gtest.h>

#include <chrono>
#include <fstream>
#include <iostream>
#include <string>

#include "orderbook/OrderBook.h"

namespace {

struct TradeCollector {
    void operator()(const ob::Trade& trade) {
        trades.push_back(trade);
    }
    std::vector<ob::Trade> trades;
};

TEST(OrderBook, BasicMatch) {
    ob::OrderBook book;
    TradeCollector collector;
    book.set_trade_callback(std::ref(collector));

    auto* sell = book.create_order("ask", 100, 10, ob::types::Side::Sell, ob::types::TimeInForce::GFD);
    ASSERT_NE(sell, nullptr);

    auto* buy = book.create_order("buy", 105, 5, ob::types::Side::Buy, ob::types::TimeInForce::IOC);
    EXPECT_EQ(buy, nullptr);

    ASSERT_EQ(collector.trades.size(), 1);
    EXPECT_EQ(collector.trades.front().traded_qty, 5);
    EXPECT_FALSE(book.has_order("buy"));
}

TEST(OrderBookPerf, OperationsWithinOneSecond) {
    using clock = std::chrono::steady_clock;
    constexpr auto duration = std::chrono::seconds{1};

    ob::OrderBook book(200'000);

    auto start = clock::now();
    auto deadline = start + duration;
    std::size_t operations = 0;
    std::size_t id_counter = 0;

    while (clock::now() < deadline) {
        std::string oid = "perf_" + std::to_string(id_counter++);
        if (id_counter == 500'000) {
            id_counter = 0;
        }

        auto* order = book.create_order(oid,
                                        100 + static_cast<ob::types::Price>(id_counter % 5),
                                        10,
                                        ob::types::Side::Buy,
                                        ob::types::TimeInForce::GFD);

        if (!order) {
            continue;
        }

        book.cancel(order->id);
        ++operations;
    }

    std::cout << "[Perf] operations completed in 1s: " << operations << '\n';

    std::ofstream log_file("perf_results.txt", std::ios::app);
    if (log_file) {
        log_file << "operations= " << operations << '\n';
    }
    EXPECT_GT(operations, 0u);
}

} // namespace
