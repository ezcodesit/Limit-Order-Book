# Tests for the matching engine built with GoogleTest

find_package(GTest REQUIRED)

add_executable(engine_tests
    EngineTests.cpp
)

target_include_directories(engine_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(engine_tests PRIVATE
    engine_lib
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(engine_tests)
