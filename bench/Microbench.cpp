#include "orderbook/OrderBook.h"

#include <chrono>
#include <iostream>
#include <random>
#include <vector>

int main() {
    constexpr std::size_t iters = 100'000;
    ob::OrderBook book(/*min_price=*/0, /*max_price=*/200'000, iters);

    std::vector<std::chrono::nanoseconds> samples;
    samples.reserve(iters);

    std::mt19937_64 rng{42};
    std::uniform_int_distribution<int> price_dist(95, 105);

    for (std::size_t i = 0; i < iters; ++i) {
        auto price = price_dist(rng);
        auto quantity = 1 + (i % 10);
        auto start = std::chrono::high_resolution_clock::now();
        book.create_order(static_cast<ob::types::OrderId>(i),
                          price,
                          quantity,
                          ob::types::Side::Buy,
                          ob::types::TimeInForce::GFD);
        auto end = std::chrono::high_resolution_clock::now();
        samples.emplace_back(end - start);
    }

    auto sum = std::chrono::nanoseconds::zero();
    for (auto ns : samples) sum += ns;
    auto mean = sum / samples.size();
    std::cout << "mean latency: " << mean.count() << " ns\n";
    return 0;
}
