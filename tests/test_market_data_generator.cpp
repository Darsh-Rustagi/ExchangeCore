#include <gtest/gtest.h>
#include "exchange/market_data_generator.hpp"

#include <filesystem>

using namespace exchange;

TEST(MarketDataGeneratorTest, Determinism) {
    GeneratorConfig cfg1{.seed = 12345, .total_events = 500};
    GeneratorConfig cfg2{.seed = 12345, .total_events = 500};

    MarketDataGenerator gen1(cfg1);
    MarketDataGenerator gen2(cfg2);

    for (std::size_t i = 0; i < 500; ++i) {
        ASSERT_TRUE(gen1.hasNext());
        ASSERT_TRUE(gen2.hasNext());

        auto e1 = gen1.next();
        auto e2 = gen2.next();

        EXPECT_EQ(e1.action, e2.action);
        EXPECT_EQ(e1.order_id, e2.order_id);
        EXPECT_EQ(e1.side, e2.side);
        EXPECT_EQ(e1.type, e2.type);
        EXPECT_EQ(e1.price, e2.price);
        EXPECT_EQ(e1.quantity, e2.quantity);
        EXPECT_EQ(e1.timestamp, e2.timestamp);
    }
}

TEST(MarketDataGeneratorTest, FileExportAndEventCounts) {
    GeneratorConfig cfg{.total_events = 100};
    MarketDataGenerator gen(cfg);

    std::filesystem::path temp_file = std::filesystem::temp_directory_path() / "test_events.jsonl";
    gen.generateToFile(temp_file);

    EXPECT_TRUE(std::filesystem::exists(temp_file));
    EXPECT_GT(std::filesystem::file_size(temp_file), 0);

    std::filesystem::remove(temp_file);
}
