find_package(GTest REQUIRED)

set(TEST_SOURCES
    test_order.cpp
    test_order_book.cpp
    test_matching_engine.cpp
    test_order_manager.cpp
    test_market_data_generator.cpp
    test_replay_engine.cpp
    test_integration.cpp
)

add_executable(run_unit_tests ${TEST_SOURCES})
target_link_libraries(run_unit_tests PRIVATE exchange_core GTest::gtest GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(run_unit_tests)
