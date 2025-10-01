# NanoBook Deep Dive

This document is the extended companion to the main README. It explains every subsystem of the NanoBook limit order book, how the pieces fit together, and walks through a full command-by-command example.

---

## 1. Architectural Overview

NanoBook follows the patterns highlighted in the CppCon 2024 talk *“When Nanoseconds Matter”*: a deterministic, single-threaded matching core per symbol, bounded lock-free queues for ingress/egress, cache-conscious data structures, and explicit control over memory management.

```
┌──────────┐   ┌────────────────┐   ┌─────────────────┐
│ stdin    │ → │ CLI Parser     │ → │ Ingress SPSC    │
└──────────┘   └────────────────┘   │ Ring Buffer     │
                                    └──────┬──────────┘
                                           │ (single consumer)
                                           ▼
                                   ┌─────────────────┐
                                   │ Matching Engine │
                                   │   (OrderBook)   │
                                   └──────┬──────────┘
                                           │
                                           ▼
                                    ┌────────────┐
                                    │ Trade Log  │─→ stdout (async)
                                    │  SPSC      │
                                    └────────────┘
```

The engine itself is intentionally single-threaded to remain deterministic and easy to reason about. Scaling across symbols means instantiating additional engines, each with their own queues.

---

## 2. Module Walkthrough

### 2.1 `include/orderbook/Types.h`
Holds type aliases and enum definitions shared across the engine:
- `Price` / `Quantity`: 64-bit integers for tick-based math.
- `Side`, `TimeInForce`: strongly typed enums.
- `OrderId`: monotonic internal identifier used in the hot path (external strings are mapped to these once by the CLI layer).

### 2.2 `include/orderbook/Order.h`
Defines the `Order` object and its embedded `OrderNode`:
- Orders carry numeric ids, integerised price/qty, side, tif, optional min fill, plus a `resting` flag indicating whether the order currently sits in the book.
- `OrderNode` links the order into intrusive FIFO lists. By embedding the node we avoid additional allocations and keep next/prev pointers cache-local with the order payload.

### 2.3 `include/orderbook/IntrusiveList.h`
Minimal intrusive FIFO used inside price levels:
- `push_back`, `front`, `pop_front`, `erase` operate on `OrderNode` pointers.
- No iterators or heap allocations; all manipulations are pointer rewrites.

### 2.4 `include/orderbook/MemoryPool.h`
Fixed-capacity allocator for `Order` instances:
- Pre-allocates aligned storage slots.
- `create()` uses placement new, `destroy()` explicitly calls the destructor.
- Prevents the hot path from touching the system heap.

### 2.5 `include/orderbook/PriceLevel.h` / `src/orderbook/PriceLevel.cpp`
Represents a single price level on one side of the book:
- Stores aggregate quantity.
- Maintains the FIFO queue of resting orders.
- `on_fill()` deducts traded quantities while keeping totals non-negative.

### 2.6 `include/orderbook/SideBook.h` / `src/orderbook/SideBook.cpp`
Manages all price levels for one side (bid or ask):
- Dense `std::vector<PriceLevel>` covering the configured price range. The vector resizes in-place when prices fall outside the current span.
- `active_` bitmap plus `best_index_` to track the top-of-book in O(1).
- `available_to()` aggregates liquidity up to a given price without touching heap structures.
- `for_each_level()` enumerates active levels (used for snapshots).

### 2.7 `include/orderbook/SpscRingBuffer.h`
Reusable single-producer/single-consumer queue:
- Requires power-of-two capacity.
- Push supports copy and move.
- Pop returns `std::optional<T>` to signal empty state.
- Memory orderings are relaxed/acquire-release appropriate for SPSC.

### 2.8 `include/orderbook/OrderBook.h` / `src/orderbook/OrderBook.cpp`
The heart of the engine:
- Holds dense `SideBook` instances plus a vector index mapping internal order id → `Order*` for constant-time lookup.
- `create_order()` allocates from the pool, indexes the order, and routes to `process()`.
- `match()` loops over resting orders:
  - Pre-checks available liquidity for FOK and MinQty using `SideBook::available_to()`.
  - Pulls the top resting order via `SideBook::best()`.
  - Trades with FIFO priority, updating both incoming and resting quantities.
  - Emits `Trade` records via a plain function pointer sink (no `std::function` overhead).
  - Removes orders when `quantity == 0`.
- Remainders: GFD orders rest on their own side; IOC remainders cancel immediately.
- `snapshot()` prints a deterministic book view by copying levels into vectors and sorting.

### 2.9 `include/engine/Engine.h` / `src/engine/Engine.cpp`
Per-symbol engine wrapper:
- Exposes a `submit()` API that maps external ids to internal numeric ids and enqueues validated commands.
- Runs a dedicated worker thread consuming the SPSC queue and a logging thread to flush trade prints.

### 2.10 `src/engine/main.cpp`
Multi-symbol dispatcher:
- Maintains a map from `<symbol>` string to `EngineApp` instance.
- Parses CLI lines in the form `SYMBOL VERB …`, forwarding commands to the appropriate engine.
- Lazily creates new engines when unseen symbols arrive, so each instrument gets its own deterministic matching loop.

### 2.11 Executables & Tests
- `src/engine/main.cpp`: entry point hooking stdin/stdout.
- `bench/Microbench.cpp`: toy harness measuring mean latency of order inserts.
- `tests/OrderBookTests.cpp`: GoogleTest verifying a basic match (more tests encouraged).

---

## 3. Build Targets

| Target | Description |
|--------|-------------|
| `orderbook_core` | Static library containing the core matching engine implementation. |
| `matching_engine` | Static library exposing the CLI-facing `EngineApp`. |
| `engine` | Command-line executable (reads stdin → matching engine). |
| `orderbook_microbench` | Legacy chrono-based benchmark (simple insertion latency probe). |
| `orderbook_bench` | Google Benchmark harness providing structured latency measurements. |
| `orderbook_tests` | GoogleTest suite. |
| `orderbook_fuzz` | Stress executable generating random traffic bursts. |

Build/test commands:
```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build
```

Run the CLI:
```bash
./build/engine
```

Run the microbench:
```bash
./build/orderbook_microbench
```

---

## 4. Command Protocol

```
BUY|SELL <TIF> <price> <qty> <id> [MIN <qty>]
CANCEL <id>
MODIFY <id> <BUY|SELL> <price> <qty> [MIN <qty>]
PRINT
```

- `TIF`: `GFD`, `IOC`, or `FOK`.
- `MIN`: optional minimum acceptable fill. If liquidity below `MIN`, order cancels.
- `MODIFY`: implemented as cancel+reenter with new parameters.
- `PRINT`: dumps the current book snapshot.

Trade prints arrive asynchronously, in the format:
```
TRADE <resting-id> <resting-price> <qty> <incoming-id> <incoming-price> <qty>
```

---

## 5. Worked Example

Let’s simulate a short session. Commands (prefixed with `>` for clarity) and outputs:

```
> AAPL SELL GFD 100 5 ask1
> AAPL SELL GFD 101 3 ask2
> AAPL BUY  GFD  99  5 bid1
> AAPL PRINT
SELL:
100 5
101 3
BUY:
99 5
```

Explanation so far:
1. `ask1` rests at 100 with quantity 5. `SideBook(Sell)` creates a price level at 100.
2. `ask2` rests at 101 with quantity 3, a new level.
3. `bid1` enters at 99; price does not cross the best ask (100) so rests on the bid side.
4. `PRINT` reads both ladders, sorting ascending for sells and descending for buys.

Now we cross the spread:

```
> AAPL BUY IOC 101 4 taker1
AAPL TRADE ask1 100 4 taker1 101 4
```

Walkthrough:
1. Incoming order `taker1` is side BUY, price 101, qty 4, IOC.
2. `OrderBook::match`
   - `available_qty` across asks at prices ≤ 101 is 8 (5 @100 + 3 @101). IOC doesn’t care—but FOK/Min would check here.
   - Pulls best ask at 100 (`ask1`).
   - Trades min(4,5) = 4 units.
   - Updates `ask1` to qty 1, adjusts level total via `on_fill()`.
   - Emits `TRADE` record; logging thread prints line asynchronously.
   - Because IOC, remainder (0) is irrelevant—no resting.

After the trade, a new print shows updated state:

```
> AAPL PRINT
SELL:
100 1
101 3
BUY:
99 5
```

Next, a Fill-or-Kill with minimum quantity:

```
> AAPL BUY FOK 101 3 taker2 MIN 3
AAPL TRADE ask1 100 1 taker2 101 1
AAPL TRADE ask2 101 2 taker2 101 2
```

Here’s the flow:
1. Incoming `taker2` (BUY, price 101, qty 3, FOK, min=3).
2. `available_qty` across asks ≤101 remains 4 (1 @100 + 3 @101). FOK requires >=3 so we proceed.
3. First trade consumes remaining 1 from `ask1`.
4. Second trade consumes 2 from `ask2`.
5. Incoming remainder 0 → order finishes successfully (FOK satisfied). Resting `ask2` now has qty 1.

Final snapshot:

```
> AAPL PRINT
SELL:
101 1
BUY:
99 5
```

`ask1` is gone, `ask2` reduced to 1, `bid1` unchanged.

Modify and cancel example:

```
> AAPL MODIFY bid1 SELL 102 4
```

This command flips `bid1` to the sell side at price 102:
1. Engine cancels original `bid1` (removing from bid ladder and releasing from pool).
2. Reenters as a fresh GFD sell order.
3. Price 102 crosses zero bids, so it rests on the ask side (now the best ask).

```
> AAPL CANCEL ask2
```

Removes `ask2`. Snapshot now shows only the modified order:

```
> AAPL PRINT
SELL:
102 4
BUY:
```

---

## 6. Performance Notes

- **Determinism:** only one thread touches the order book. Thread handoff happens via lock-free queues.
- **Memory locality:** Orders embed their list node and are pool-allocated, keeping related data contiguous.
- **Logging:** trade prints leave the critical section immediately. Replace the logging thread’s `std::cout` with a custom sink for production.
- **Extensions:** bounded tick markets can swap the map/set structure for a dense vector+bitmap, as suggested in the CppCon talk.

---

## 7. Next Steps

1. Implement reader snapshots via double-buffering or RCU-style copy-out.
2. Add more thorough tests (multi-level matches, modify edge cases, min-qty failures).
3. Expand the benchmark harness to compute percentile latency and integrate with perf counters.
4. Run clang-tidy / clang-format as part of CI.

With this reference you should be able to explore every component of NanoBook, understand how commands flow through the system, and extend it with confidence.
