#include <gtest/gtest.h>
#include "exchange/order_manager.hpp"
#include "exchange/matching_engine.hpp"
#include "exchange/execution_reporter.hpp"
#include "exchange/market_data_generator.hpp"
#include "exchange/replay_engine.hpp"

#include <filesystem>

using namespace exchange;

TEST(ReplayEngineTest, DeterministicReplayMatchesExactMetrics) {
    GeneratorConfig cfg{
        .symbol = "AAPL",
        .base_price = 15000,
        .price_tick = 5,
        .seed = 999,
        .total_events = 2000
    };

    std::filesystem::path temp_stream = std::filesystem::temp_directory_path() / "replay_stream_test.jsonl";
    {
        MarketDataGenerator gen(cfg);
        gen.generateToFile(temp_stream);
    }

    OrderManager om1;
    ExecutionReporter rep1;
    MatchingEngine me1(om1, rep1);
    ReplayEngine replay1(om1, me1, rep1);
    auto stats1 = replay1.replayFromFile(temp_stream);

    OrderManager om2;
    ExecutionReporter rep2;
    MatchingEngine me2(om2, rep2);
    ReplayEngine replay2(om2, me2, rep2);
    auto stats2 = replay2.replayFromFile(temp_stream);

    EXPECT_EQ(stats1.orders_submitted, stats2.orders_submitted);
    EXPECT_EQ(stats1.orders_cancelled, stats2.orders_cancelled);
    EXPECT_EQ(stats1.trades_executed, stats2.trades_executed);
    EXPECT_EQ(stats1.shares_traded, stats2.shares_traded);
    EXPECT_EQ(stats1.volume_value_cents, stats2.volume_value_cents);
    EXPECT_GT(stats1.trades_executed, 0);

    std::filesystem::remove(temp_stream);
}
