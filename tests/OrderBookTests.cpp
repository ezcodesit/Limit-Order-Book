#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <iostream>
#include <vector>

#include "orderbook/OrderBook.h"

namespace {

struct TradeCollector {
    static void sink(const ob::Trade& trade, void* ctx) {
        auto* self = static_cast<TradeCollector*>(ctx);
        self->trades.push_back(trade);
    }
    std::vector<ob::Trade> trades;
};

struct TradeCounter {
    static void sink(const ob::Trade& trade, void* ctx) {
        auto* self = static_cast<TradeCounter*>(ctx);
        ++self->trades;
        if (trade.traded_qty > 0) {
            self->filled_quantity += static_cast<std::uint64_t>(trade.traded_qty);
        }
    }
    std::uint64_t trades{0};
    std::uint64_t filled_quantity{0};
};

TEST(OrderBook, BasicMatch) {
    ob::OrderBook book(/*min_price=*/90, /*max_price=*/110);
    TradeCollector collector;
    book.set_trade_sink(&TradeCollector::sink, &collector);

    auto* sell = book.create_order(1, 100, 10, ob::types::Side::Sell, ob::types::TimeInForce::GFD);
    ASSERT_NE(sell, nullptr);

    auto* buy = book.create_order(2, 105, 5, ob::types::Side::Buy, ob::types::TimeInForce::IOC);
    EXPECT_EQ(buy, nullptr);

    ASSERT_EQ(collector.trades.size(), 1);
    EXPECT_EQ(collector.trades.front().traded_qty, 5);
    EXPECT_FALSE(book.has_order(2));
}

TEST(OrderBook, MultiLevelMatchAndRest) {
    ob::OrderBook book(/*min_price=*/95, /*max_price=*/105);
    TradeCollector collector;
    book.set_trade_sink(&TradeCollector::sink, &collector);

    auto* ask1 = book.create_order(1, 100, 5, ob::types::Side::Sell, ob::types::TimeInForce::GFD);
    ASSERT_NE(ask1, nullptr);
    auto* ask2 = book.create_order(2, 101, 7, ob::types::Side::Sell, ob::types::TimeInForce::GFD);
    ASSERT_NE(ask2, nullptr);
    auto* ask3 = book.create_order(3, 102, 3, ob::types::Side::Sell, ob::types::TimeInForce::GFD);
    ASSERT_NE(ask3, nullptr);

    auto* incoming = book.create_order(4, 102, 12, ob::types::Side::Buy, ob::types::TimeInForce::GFD);
    ASSERT_NE(incoming, nullptr);

    ASSERT_EQ(collector.trades.size(), 2);
    EXPECT_EQ(collector.trades[0].resting_id, 1u);
    EXPECT_EQ(collector.trades[0].traded_qty, 5);
    EXPECT_EQ(collector.trades[1].resting_id, 2u);
    EXPECT_EQ(collector.trades[1].traded_qty, 7);

    EXPECT_FALSE(book.has_order(1));
    EXPECT_FALSE(book.has_order(2));
    EXPECT_TRUE(book.has_order(3));
    EXPECT_TRUE(book.has_order(4));

    auto* remainder = book.find(4);
    ASSERT_NE(remainder, nullptr);
    EXPECT_TRUE(remainder->resting);
    EXPECT_EQ(remainder->quantity, 0);
}

TEST(OrderBook, FillOrKillRejection) {
    ob::OrderBook book(/*min_price=*/90, /*max_price=*/110);
    auto* ask = book.create_order(1, 100, 3, ob::types::Side::Sell, ob::types::TimeInForce::GFD);
    ASSERT_NE(ask, nullptr);

    auto* fok = book.create_order(2, 101, 5, ob::types::Side::Buy, ob::types::TimeInForce::FOK);
    EXPECT_EQ(fok, nullptr);
    EXPECT_FALSE(book.has_order(2));
    EXPECT_TRUE(book.has_order(1));
}

TEST(OrderBook, MinimumQuantityEnforced) {
    ob::OrderBook book(/*min_price=*/90, /*max_price=*/110);
    auto* ask = book.create_order(1, 100, 8, ob::types::Side::Sell, ob::types::TimeInForce::GFD);
    ASSERT_NE(ask, nullptr);

    auto* buy = book.create_order(2,
                                  100,
                                  8,
                                  ob::types::Side::Buy,
                                  ob::types::TimeInForce::GFD,
                                  std::optional<ob::types::Quantity>(10));
    EXPECT_EQ(buy, nullptr);
    EXPECT_TRUE(book.has_order(1));
}

TEST(OrderBook, ModifySideAndTimeInForce) {
    ob::OrderBook book(/*min_price=*/90, /*max_price=*/110);
    auto* order = book.create_order(1, 100, 5, ob::types::Side::Buy, ob::types::TimeInForce::GFD);
    ASSERT_NE(order, nullptr);
    EXPECT_TRUE(order->resting);

    book.modify(1,
                ob::types::Side::Sell,
                101,
                5,
                ob::types::TimeInForce::IOC,
                std::optional<ob::types::Quantity>(3));

    const ob::Order* modified = book.find(1);
    ASSERT_NE(modified, nullptr);
    EXPECT_EQ(modified->side, ob::types::Side::Sell);
    EXPECT_EQ(modified->tif, ob::types::TimeInForce::IOC);
    EXPECT_TRUE(modified->has_min_qty);
    EXPECT_EQ(modified->min_qty, 3);
}

TEST(OrderBookPerf, OperationsWithinOneSecond) {
    using clock = std::chrono::steady_clock;
    constexpr auto duration = std::chrono::seconds{1};

    ob::OrderBook book(/*min_price=*/0, /*max_price=*/200'000, /*pool_capacity=*/600'000);

    auto start = clock::now();
    auto deadline = start + duration;
    std::size_t operations = 0;
    ob::types::OrderId id_counter = 0;

    while (clock::now() < deadline) {
        auto current_id = id_counter++;
        if (id_counter == 500'000) {
            id_counter = 0;
        }

        auto* order = book.create_order(current_id,
                                        100 + static_cast<ob::types::Price>(current_id % 5),
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
    EXPECT_GT(operations, 0u);
}

TEST(OrderBookPerf, ExecutionsWithinOneSecond) {
    using clock = std::chrono::steady_clock;
    constexpr auto duration = std::chrono::seconds{1};

    ob::OrderBook book(/*min_price=*/90, /*max_price=*/110, /*pool_capacity=*/2'000'000);
    TradeCounter counter;
    book.set_trade_sink(&TradeCounter::sink, &counter);

    auto* resting_liquidity = book.create_order(0,
                                                100,
                                                50'000'000,
                                                ob::types::Side::Sell,
                                                ob::types::TimeInForce::GFD);
    ASSERT_NE(resting_liquidity, nullptr);

    auto start = clock::now();
    auto deadline = start + duration;
    ob::types::OrderId next_id = 1;
    constexpr ob::types::OrderId max_id = 500'000;

    while (clock::now() < deadline) {
        auto id = next_id++;
        if (next_id == max_id) {
            next_id = 1;
        }
        (void)book.create_order(id,
                                101,
                                1,
                                ob::types::Side::Buy,
                                ob::types::TimeInForce::IOC);
        if (book.has_order(id)) {
            book.cancel(id);
        }
    }

    std::cout << "[Perf] executions completed in 1s: "
              << counter.trades << " trades, total quantity "
              << counter.filled_quantity << '\n';

    EXPECT_GT(counter.trades, 0u);
    EXPECT_GT(counter.filled_quantity, 0u);
}

} // namespace
