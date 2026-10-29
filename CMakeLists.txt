cmake_minimum_required(VERSION 3.20)
project(ExchangeCore VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang|GNU")
    add_compile_options(-Wall -Wextra -Wpedantic -Wconversion -Wshadow)
endif()

list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew")

add_library(exchange_core
    src/order_book.cpp
    src/matching_engine.cpp
    src/execution_reporter.cpp
    src/order_manager.cpp
    src/event_log.cpp
    src/market_data_generator.cpp
    src/replay_engine.cpp
)

target_include_directories(exchange_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_compile_features(exchange_core PUBLIC cxx_std_20)

add_executable(exchange_main src/main.cpp)
target_link_libraries(exchange_main PRIVATE exchange_core)

enable_testing()
add_subdirectory(tests)
add_subdirectory(benchmarks)
