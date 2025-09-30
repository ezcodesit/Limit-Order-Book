# NanoBook

High-performance, single-threaded limit order book inspired by the CppCon 2024 talk **When Nanoseconds Matter**. The project is structured for clarity first, but follows patterns that keep the hot path cache-friendly and free of blocking syscalls.

## Architecture Overview
- **Deterministic engine**: a single matching loop per symbol, fed via lock-free SPSC ring buffers.
- **Order storage**: dense price ladder backed by contiguous `PriceLevel` slots; each level embeds an intrusive FIFO of resting orders to maintain price-time priority.
- **Numeric IDs**: external string ids are mapped once to integral ids so the hot path never touches `std::string` or hashing.
- **Memory pool**: fixed-capacity allocator avoids heap traffic on the matching path.
- **Snapshots**: future double-buffer API to expose reader-friendly copies with zero contention.

## Getting Started
```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build
```

Targets:
- `orderbook_core`: core matching engine library.
- `matching_engine`: CLI wrapper + ring-buffer ingestion.
- `engine`: command-line executable (reads stdin commands).
- `orderbook_tests`: GoogleTest suite covering core flows.

## Command Protocol
```
BUY|SELL <TIF> <price> <qty> <id> [MIN <qty>]
CANCEL <id>
MODIFY <id> <BUY|SELL> <price> <qty> [MIN <qty>]
PRINT
```
Outputs `TRADE ...` lines for each execution; `PRINT` emits an aggregated snapshot of the book.

## Future Work
- Double-buffered snapshots for readers.
- Async logging thread already queues trades off the hot path; extend to durable sinks.
- Dense vector price ladder for bounded markets.
- Microbench harness (chrono-based) to profile latency distribution.
