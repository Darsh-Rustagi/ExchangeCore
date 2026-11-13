#include <gtest/gtest.h>
#include "exchange/types.hpp"
#include "exchange/order_manager.hpp"
#include "exchange/matching_engine.hpp"
#include "exchange/execution_reporter.hpp"
#include "exchange/event_log.hpp"
#include "exchange/market_data_generator.hpp"
#include "exchange/replay_engine.hpp"

#include <filesystem>

using namespace exchange;

TEST(IntegrationTest, FullLifecycleWithEventLogRoundTrip) {
    std::filesystem::path event_log_path = std::filesystem::temp_directory_path() / "integration_events.jsonl";

    {
        EventLog event_log(event_log_path);
        OrderManager order_mgr;
        ExecutionReporter reporter(&event_log);
        MatchingEngine engine(order_mgr, reporter);

        GeneratorConfig config{
            .symbol = "AAPL",
            .base_price = 15000,
            .price_tick = 5,
            .market_order_ratio = 0.15,
            .cancel_ratio = 0.10,
            .seed = 777,
            .total_events = 1000
        };

        MarketDataGenerator gen(config);
        ReplayEngine replay(order_mgr, engine, reporter);
        auto stats = replay.replayFromGenerator(gen);

        EXPECT_GT(stats.orders_submitted, 0);
        EXPECT_GT(stats.trades_executed, 0);
        event_log.flush();
    }

    ASSERT_TRUE(std::filesystem::exists(event_log_path));
    auto events = EventLog::loadEvents(event_log_path);
    EXPECT_GT(events.size(), 0);

    std::size_t accepted_count = 0;
    std::size_t trade_count = 0;
    std::size_t cancel_count = 0;

    for (const auto& ev : events) {
        if (ev.type == Event::Type::OrderAccepted) accepted_count++;
        else if (ev.type == Event::Type::TradeExecuted) trade_count++;
        else if (ev.type == Event::Type::OrderCancelled) cancel_count++;
    }

    EXPECT_GT(accepted_count, 0);
    EXPECT_GT(trade_count, 0);

    std::filesystem::remove(event_log_path);
}

TEST(IntegrationTest, MultiSymbolInterleavedStressTest) {
    OrderManager order_mgr;
    ExecutionReporter reporter;
    MatchingEngine engine(order_mgr, reporter);

    std::vector<std::string> symbols = {"AAPL", "MSFT", "GOOG", "AMZN"};
    OrderId start_id = 1;

    for (const auto& sym : symbols) {
        GeneratorConfig cfg{
            .symbol = sym,
            .base_price = 20000,
            .price_tick = 10,
            .seed = 42,
            .start_order_id = start_id,
            .total_events = 250
        };
        start_id += 500;
        MarketDataGenerator gen(cfg);
        ReplayEngine replay(order_mgr, engine, reporter);
        replay.replayFromGenerator(gen);
    }

    for (const auto& sym : symbols) {
        auto* book = engine.getBook(sym);
        ASSERT_NE(book, nullptr);
        EXPECT_EQ(book->symbol(), sym);
    }
}
