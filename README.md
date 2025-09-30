# Order Book Engine

A minimal limit order book engine that supports BUY/SELL entry, matching, cancellation, modification, and book printing. The project includes a small command-line driver plus GoogleTest-based unit tests.

## Build Requirements

- CMake 3.10+
- A C++17 compliant compiler (tested with AppleClang 15)
- GoogleTest (available via package manager, e.g. `brew install googletest`)

## Configure, Build, and Test

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build
```

These commands produce the `engine` executable and run the unit test suite (`engine_tests`).

## Usage

Feed commands to the `engine` executable via standard input. Supported operations:

- `BUY <TIF> <price> <qty> <id>`
- `SELL <TIF> <price> <qty> <id>`
- `CANCEL <id>`
- `MODIFY <id> <BUY|SELL> <price> <qty>`
- `PRINT`

Example session:

```bash
./build/engine <<'CMDS'
SELL GFD 100 5 s1
BUY IOC 110 3 b1
PRINT
CMDS
```

Modify commands only apply to resting orders; setting quantity to zero or providing an invalid price removes the order.

## Project Structure

- `include/` – public headers (`Engine.h`, `Order.h`, `Info.h`)
- `src/` – engine implementation and CLI driver
- `tests/` – GoogleTest suite covering core book behaviours

Contributions welcome. Please run the test suite before submitting changes.
