#include "orderbook/OrderBook.h"

#include <benchmark/benchmark.h>

namespace {
constexpr ob::types::Price kMinPrice = 0;
constexpr ob::types::Price kMaxPrice = 200'000;
constexpr std::size_t      kPool     = 1'000'000;
}

static void BM_Insert(benchmark::State& state) {
    ob::OrderBook book(kMinPrice, kMaxPrice, kPool);
    ob::types::OrderId order_id = 0;
    for (auto _ : state) {
        benchmark::DoNotOptimize(book.create_order(order_id++,
                                                   100,
                                                   10,
                                                   ob::types::Side::Buy,
                                                   ob::types::TimeInForce::GFD));
        if (order_id % 1024 == 0) {
            book.cancel(order_id - 1);
        }
    }
}
BENCHMARK(BM_Insert);

static void BM_Match(benchmark::State& state) {
    ob::OrderBook book(kMinPrice, kMaxPrice, kPool);
    ob::types::OrderId id = 0;
    for (int i = 0; i < 5000; ++i) {
        book.create_order(id++, 100, 10, ob::types::Side::Sell, ob::types::TimeInForce::GFD);
    }
    for (auto _ : state) {
        book.create_order(id++, 100, 10, ob::types::Side::Buy, ob::types::TimeInForce::IOC);
    }
}
BENCHMARK(BM_Match);

static void BM_Cancel(benchmark::State& state) {
    ob::OrderBook book(kMinPrice, kMaxPrice, kPool);
    ob::types::OrderId id = 0;
    std::vector<ob::types::OrderId> ids;
    ids.reserve(10'000);
    for (auto _ : state) {
        for (int i = 0; i < 100; ++i) {
            auto current = id++;
            book.create_order(current, 100 + (current % 10), 10,
                              ob::types::Side::Buy, ob::types::TimeInForce::GFD);
            ids.push_back(current);
        }
        for (auto cancel_id : ids) book.cancel(cancel_id);
        ids.clear();
    }
}
BENCHMARK(BM_Cancel);

BENCHMARK_MAIN();
