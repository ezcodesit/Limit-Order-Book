cmake_minimum_required(VERSION 3.10)
project(Engine)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# core order book library
add_library(orderbook_core STATIC
    src/orderbook/OrderBook.cpp
    src/orderbook/SideBook.cpp
    src/orderbook/PriceLevel.cpp
)
target_include_directories(orderbook_core PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

# runtime engine wrapper
add_library(matching_engine STATIC
    src/engine/Engine.cpp
)
target_link_libraries(matching_engine PUBLIC orderbook_core)
target_include_directories(matching_engine PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

# main engine executable
add_executable(engine
    src/engine/main.cpp
)
target_link_libraries(engine PRIVATE matching_engine)

add_executable(orderbook_microbench
    bench/Microbench.cpp
)
target_link_libraries(orderbook_microbench PRIVATE orderbook_core)

# Google Benchmark (optional)
find_package(benchmark QUIET)
if(benchmark_FOUND)
    add_executable(orderbook_bench
        bench/OrderBookBench.cpp
    )
    target_link_libraries(orderbook_bench PRIVATE orderbook_core benchmark::benchmark)
endif()

add_executable(orderbook_fuzz
    tests/FuzzHarness.cpp
)
target_link_libraries(orderbook_fuzz PRIVATE orderbook_core)

enable_testing()
add_subdirectory(tests)
