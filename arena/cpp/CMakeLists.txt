cmake_minimum_required(VERSION 3.20)
project(orderbook_arena_cpp_strategies LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(
    arena_simple_reference_strategy
    examples/simple_reference_strategy.cpp
)

target_include_directories(
    arena_simple_reference_strategy
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

if(MSVC)
    target_compile_options(arena_simple_reference_strategy PRIVATE /W4)
else()
    target_compile_options(
        arena_simple_reference_strategy
        PRIVATE
            -Wall
            -Wextra
            -Wpedantic
    )
endif()
