find_package(GTest QUIET)

if(NOT GTest_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
endif()

add_executable(orderbook_tests
    OrderBookTests.cpp
)

target_link_libraries(orderbook_tests PRIVATE
    orderbook_core
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(orderbook_tests)
