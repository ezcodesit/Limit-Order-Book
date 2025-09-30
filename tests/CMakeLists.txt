find_package(GTest REQUIRED)

add_executable(orderbook_tests
    OrderBookTests.cpp
)

target_link_libraries(orderbook_tests PRIVATE
    orderbook_core
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(orderbook_tests)
