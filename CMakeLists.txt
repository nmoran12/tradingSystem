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
    src/benchmarks/WorkloadGenerator.cpp
    src/protocol/BinaryProtocol.cpp
    src/protocol/BinaryCommandReader.cpp
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
    tests/test_order_book_reserve.cpp
    tests/test_latency_tracker.cpp
    tests/test_matching_engine.cpp
    tests/test_matching_engine_process_into.cpp
    tests/test_order_command_parser.cpp
    tests/test_market_and_modify_orders.cpp
    tests/test_workload_generator.cpp
    tests/test_binary_command_reader.cpp
    tests/test_cli_binary_engine.cpp
    tests/test_binary_protocol.cpp
    tests/test_spsc_ring_buffer.cpp
    tests/test_spsc_command_pipeline.cpp
)

add_executable(matching_engine_benchmark benchmarks/benchmark_matching_engine.cpp)
target_link_libraries(matching_engine_benchmark PRIVATE orderbook_core)

add_executable(binary_protocol_benchmark benchmarks/benchmark_binary_protocol.cpp)
target_link_libraries(binary_protocol_benchmark PRIVATE orderbook_core)

target_link_libraries(orderbook_tests
    PRIVATE
        orderbook_core
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(orderbook_tests)
