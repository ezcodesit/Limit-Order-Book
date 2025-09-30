#include "orderbook/OrderBook.h"

#include <chrono>
#include <iostream>
#include <random>

int main() {
    constexpr std::size_t iters = 100'000;
    ob::OrderBook book(iters);

    std::vector<std::chrono::nanoseconds> samples;
    samples.reserve(iters);

    std::mt19937_64 rng{42};
    std::uniform_int_distribution<int> price_dist(95, 105);

    for (std::size_t i = 0; i < iters; ++i) {
        auto id = "order_" + std::to_string(i);
        auto price = price_dist(rng);
        auto quantity = 1 + (i % 10);
        auto start = std::chrono::high_resolution_clock::now();
        book.create_order(id, price, quantity, ob::types::Side::Buy, ob::types::TimeInForce::GFD);
        auto end = std::chrono::high_resolution_clock::now();
        samples.emplace_back(end - start);
    }

    auto sum = std::chrono::nanoseconds::zero();
    for (auto ns : samples) sum += ns;
    auto mean = sum / samples.size();
    std::cout << "mean latency: " << mean.count() << " ns\n";
    return 0;
}
