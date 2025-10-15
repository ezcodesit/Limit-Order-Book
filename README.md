# NanoBook

High-performance, single-threaded limit order book inspired by the CppCon 2024 talk **When Nanoseconds Matter**. NanoBook keeps the matching path deterministic, cache-friendly, and allocation-free while remaining easy to reason about.

## Highlights
- **Deterministic core** – one matching loop per symbol, fed by lock-free SPSC ring buffers.
- **Cache-conscious data** – dense `SideBook` price ladders with intrusive FIFOs (optional Boost.Intrusive drop-in via `-DUSE_BOOST_INTRUSIVE=ON`).
- **Zero-heap hot path** – fixed-capacity `MemoryPool` plus numeric order IDs mapped from client strings.
- **Trade sink hooks** – pluggable callback for logging or metrics; async logger thread in the CLI wrapper.
- **Execution perf probe** – counts real trades per second (not just create/cancel churn).

## Quick Start
```bash
# Debug build with tests
cmake -S . -B build
cmake --build build
ctest --test-dir build

# Release build (recommended for perf numbers)
cmake -S . -B build-rel -DCMAKE_BUILD_TYPE=Release
cmake --build build-rel
```

Optional Boost FIFO:
```bash
cmake -S . -B build-rel -DCMAKE_BUILD_TYPE=Release -DUSE_BOOST_INTRUSIVE=ON
```

### Targets
- `orderbook_core` – matching engine library.
- `matching_engine` – CLI-oriented wrapper (ID mapping, ingress/egress threads).
- `engine` – stdin-driven executable using the wrapper.
- `orderbook_tests` – GoogleTest suite (core flows + perf probes).
- `orderbook_fuzz` – random stress generator over the `OrderBook` API.
- `orderbook_microbench` / `orderbook_bench` – simple chrono benchmark and Google Benchmark harness (optional).

## Performance Snapshots
Release build with GCC 13 on a development workstation:
```bash
ctest --test-dir build-rel -R OrderBookPerf.OperationsWithinOneSecond -V
# ~24M create+cancel cycles / second

ctest --test-dir build-rel -R OrderBookPerf.ExecutionsWithinOneSecond -V
# ~19M fully executed IOC trades / second (prefilled sell wall)
```

Both tests default to the hand-written intrusive FIFO; flip `-DUSE_BOOST_INTRUSIVE=ON` to compare against `boost::intrusive::list`.

## Using the CLI

Run the engine (reads commands from stdin):
```bash
./build-rel/engine         # use ./build/engine if you built the Debug profile
```

Optional helpers:
```bash
./build-rel/orderbook_bench        # Google Benchmark suite (if available)
./build-rel/orderbook_fuzz         # random stress test with default seed
./build-rel/orderbook_fuzz 123456  # same, with custom seed
ctest --test-dir build-rel -R OrderBookPerf.* -V  # run perf probes only
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

## Architecture Overview
- **Deterministic engine**: a single matching loop per symbol, fed via lock-free SPSC ring buffers.
- **Order storage**: dense price ladder backed by contiguous `PriceLevel` slots; each level embeds an intrusive FIFO of resting orders to maintain price-time priority.
- **Numeric IDs**: external string IDs are mapped once to integral IDs so the hot path never touches `std::string` or hashing.
- **Memory pool**: fixed-capacity allocator avoids heap traffic on the matching path.
- **Observability**: simple trade-sink hook plus async logging thread in the CLI wrapper.

## Testing
- `ctest --test-dir build-rel` – run the full GoogleTest suite.
- `ctest --test-dir build-rel -R OrderBookPerf.* -V` – perf probes (create/cancel + execution throughput).
- `./build-rel/orderbook_fuzz [seed]` – randomized add/cancel/modify loop with basic invariants.

## Future Work
- Double-buffered snapshots for readers.
- Async logging thread already queues trades off the hot path; extend to durable sinks.
- More realistic replay harness (CSV or binary feed) to drive the engine.
- Optional market/stop order types layered on top of the core book.
