cmake_minimum_required(VERSION 3.20)
project(cpp-low-latency-orderbook VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(orderbook_core
    src/market_data/MarketEvent.cpp
    src/market_data/MarketDataParser.cpp
    src/market_data/OrderCommandParser.cpp
    src/order_book/OrderBook.cpp
    src/matching_engine/MatchingEngine.cpp
    src/matching_engine/EngineEventPrinter.cpp
    src/metrics/LatencyTracker.cpp
)

target_include_directories(orderbook_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(cpp-low-latency-orderbook src/main.cpp)
target_link_libraries(cpp-low-latency-orderbook PRIVATE orderbook_core)

enable_testing()

include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(orderbook_tests
    tests/test_market_data_parser.cpp
    tests/test_order_book.cpp
    tests/test_latency_tracker.cpp
    tests/test_matching_engine.cpp
    tests/test_order_command_parser.cpp
    tests/test_market_and_modify_orders.cpp
)

target_link_libraries(orderbook_tests
    PRIVATE
        orderbook_core
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(orderbook_tests)
