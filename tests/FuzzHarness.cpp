#include "orderbook/OrderBook.h"

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <random>
#include <vector>

int main(int argc, char** argv) {
    std::uint64_t seed = 42;
    if (argc > 1) seed = std::strtoull(argv[1], nullptr, 10);

    std::mt19937_64 rng(seed);
    std::uniform_int_distribution<int> side_dist(0, 1);
    std::uniform_int_distribution<int> price_dist(90, 110);
    std::uniform_int_distribution<int> qty_dist(1, 25);
    std::uniform_int_distribution<int> tif_dist(0, 2);
    std::uniform_int_distribution<int> action_dist(0, 2);

    constexpr std::size_t iterations = 1'000'000;

    ob::OrderBook book(/*min_price=*/0, /*max_price=*/200'000, /*pool_capacity=*/2'000'000);
    std::vector<ob::types::OrderId> live_ids;
    live_ids.reserve(1'000'000);
    ob::types::OrderId next_id = 0;

    auto start = std::chrono::steady_clock::now();

    for (std::size_t i = 0; i < iterations; ++i) {
        int action = action_dist(rng);
        if (action == 0 || live_ids.empty()) {
            auto id = next_id++;
            auto price = price_dist(rng);
            auto qty = qty_dist(rng);
            auto side = side_dist(rng) == 0 ? ob::types::Side::Buy : ob::types::Side::Sell;
            ob::types::TimeInForce tif = static_cast<ob::types::TimeInForce>(tif_dist(rng));
            book.create_order(id, price, qty, side, tif);
            live_ids.push_back(id);
        } else if (action == 1) {
            auto idx = rng() % live_ids.size();
            book.cancel(live_ids[idx]);
            live_ids[idx] = live_ids.back();
            live_ids.pop_back();
        } else {
            auto idx = rng() % live_ids.size();
            auto id = live_ids[idx];
            auto price = price_dist(rng);
            auto qty = qty_dist(rng);
            book.modify(id, price, qty);
        }
    }

    auto end = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "Fuzz completed in " << elapsed.count() << " ms with "
              << live_ids.size() << " live orders remaining" << std::endl;

    return 0;
}
