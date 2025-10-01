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
- `orderbook_bench`: Google Benchmark harness for micro latency tests.
- `orderbook_fuzz`: stress executable that hammers the engine with randomised traffic.

## Using the CLI

Run the engine (reads commands from stdin):
```bash
./build/engine
```

Optional helpers:
```bash
./build/orderbook_bench        # runs Google Benchmark suite (if available)
./build/orderbook_fuzz         # random stress test with default seed
./build/orderbook_fuzz 123456  # same, with custom seed
```

## Command Protocol
```
<symbol> BUY|SELL <TIF> <price> <qty> <client-id> [MIN <qty>]
<symbol> CANCEL <client-id>
<symbol> MODIFY <client-id> <BUY|SELL> <price> <qty> [MIN <qty>]
<symbol> PRINT
```

- `symbol`: arbitrary identifier for the instrument; each symbol gets its own matching loop.
- `TIF`: `GFD`, `IOC`, or `FOK`. `MIN <qty>` enforces a minimum acceptable fill before resting; if liquidity is below the threshold the order cancels.
- Trade prints include the symbol prefix, e.g. `AAPL TRADE ...`. `PRINT` emits a snapshot for the specified symbol.

## Future Work
- Double-buffered snapshots for readers.
- Async logging thread already queues trades off the hot path; extend to durable sinks.
- Dense vector price ladder for bounded markets.
- Microbench harness (chrono-based) to profile latency distribution.
