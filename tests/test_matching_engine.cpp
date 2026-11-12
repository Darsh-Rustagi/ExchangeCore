#include <gtest/gtest.h>
#include "exchange/types.hpp"
#include "exchange/order_manager.hpp"
#include "exchange/matching_engine.hpp"
#include "exchange/execution_reporter.hpp"

using namespace exchange;

class MatchingEngineTest : public ::testing::Test {
protected:
    OrderManager order_mgr;
    ExecutionReporter reporter;
    MatchingEngine engine{order_mgr, reporter};
    std::vector<Trade> trades;

    void SetUp() override {
        reporter.onTrade([this](const Trade& t) {
            trades.push_back(t);
        });
    }
};

TEST_F(MatchingEngineTest, Scenario1_ExactMatch) {
    auto* sell = order_mgr.createOrder(Side::Sell, OrderType::Limit, "AAPL", 5000, 100);
    engine.processOrder(sell);

    auto* buy = order_mgr.createOrder(Side::Buy, OrderType::Limit, "AAPL", 5000, 100);
    engine.processOrder(buy);

    ASSERT_EQ(trades.size(), 1);
    EXPECT_EQ(trades[0].buy_order_id, buy->id);
    EXPECT_EQ(trades[0].sell_order_id, sell->id);
    EXPECT_EQ(trades[0].price, 5000);
    EXPECT_EQ(trades[0].quantity, 100);

    EXPECT_EQ(buy->status, OrderStatus::Filled);
    EXPECT_EQ(sell->status, OrderStatus::Filled);
    EXPECT_EQ(engine.getBook("AAPL")->totalOrderCount(), 0);
}

TEST_F(MatchingEngineTest, Scenario2_PartialFillIncoming) {
    auto* sell = order_mgr.createOrder(Side::Sell, OrderType::Limit, "AAPL", 5000, 100);
    engine.processOrder(sell);

    auto* buy = order_mgr.createOrder(Side::Buy, OrderType::Limit, "AAPL", 5000, 40);
    engine.processOrder(buy);

    ASSERT_EQ(trades.size(), 1);
    EXPECT_EQ(trades[0].quantity, 40);
    EXPECT_EQ(buy->status, OrderStatus::Filled);
    EXPECT_EQ(sell->status, OrderStatus::PartiallyFilled);
    EXPECT_EQ(sell->remaining(), 60);

    EXPECT_EQ(engine.getBook("AAPL")->totalOrderCount(), 1);
    EXPECT_EQ(engine.getBook("AAPL")->peekBestAsk()->remaining(), 60);
}

TEST_F(MatchingEngineTest, Scenario3_PartialFillResting) {
    auto* sell = order_mgr.createOrder(Side::Sell, OrderType::Limit, "AAPL", 5000, 40);
    engine.processOrder(sell);

    auto* buy = order_mgr.createOrder(Side::Buy, OrderType::Limit, "AAPL", 5000, 100);
    engine.processOrder(buy);

    ASSERT_EQ(trades.size(), 1);
    EXPECT_EQ(trades[0].quantity, 40);
    EXPECT_EQ(sell->status, OrderStatus::Filled);
    EXPECT_EQ(buy->status, OrderStatus::PartiallyFilled);
    EXPECT_EQ(buy->remaining(), 60);

    EXPECT_EQ(engine.getBook("AAPL")->totalOrderCount(), 1);
    EXPECT_EQ(engine.getBook("AAPL")->peekBestBid()->id, buy->id);
    EXPECT_EQ(engine.getBook("AAPL")->peekBestBid()->remaining(), 60);
}

TEST_F(MatchingEngineTest, Scenario4_MultiLevelMatch) {
    auto* s1 = order_mgr.createOrder(Side::Sell, OrderType::Limit, "AAPL", 5000, 40);
    engine.processOrder(s1);
    auto* s2 = order_mgr.createOrder(Side::Sell, OrderType::Limit, "AAPL", 5200, 60);
    engine.processOrder(s2);

    auto* buy = order_mgr.createOrder(Side::Buy, OrderType::Limit, "AAPL", 5500, 100);
    engine.processOrder(buy);

    ASSERT_EQ(trades.size(), 2);
    EXPECT_EQ(trades[0].sell_order_id, s1->id);
    EXPECT_EQ(trades[0].price, 5000);
    EXPECT_EQ(trades[0].quantity, 40);

    EXPECT_EQ(trades[1].sell_order_id, s2->id);
    EXPECT_EQ(trades[1].price, 5200);
    EXPECT_EQ(trades[1].quantity, 60);

    EXPECT_EQ(buy->status, OrderStatus::Filled);
    EXPECT_EQ(engine.getBook("AAPL")->totalOrderCount(), 0);
}

TEST_F(MatchingEngineTest, Scenario5_NoMatch) {
    auto* buy = order_mgr.createOrder(Side::Buy, OrderType::Limit, "AAPL", 4800, 100);
    engine.processOrder(buy);

    auto* sell = order_mgr.createOrder(Side::Sell, OrderType::Limit, "AAPL", 5000, 100);
    engine.processOrder(sell);

    EXPECT_EQ(trades.size(), 0);
    EXPECT_EQ(buy->status, OrderStatus::New);
    EXPECT_EQ(sell->status, OrderStatus::New);
    EXPECT_EQ(engine.getBook("AAPL")->totalOrderCount(), 2);
    EXPECT_EQ(engine.getBook("AAPL")->spread(), 200);
}

TEST_F(MatchingEngineTest, Scenario6_PriceImprovement) {
    auto* sell = order_mgr.createOrder(Side::Sell, OrderType::Limit, "AAPL", 5000, 50);
    engine.processOrder(sell);

    auto* buy = order_mgr.createOrder(Side::Buy, OrderType::Limit, "AAPL", 5500, 50);
    engine.processOrder(buy);

    ASSERT_EQ(trades.size(), 1);
    EXPECT_EQ(trades[0].price, 5000);
}

TEST_F(MatchingEngineTest, Scenario7_TimePriority) {
    auto* sA = order_mgr.createOrder(Side::Sell, OrderType::Limit, "AAPL", 5000, 50);
    engine.processOrder(sA);
    auto* sB = order_mgr.createOrder(Side::Sell, OrderType::Limit, "AAPL", 5000, 50);
    engine.processOrder(sB);

    auto* buy = order_mgr.createOrder(Side::Buy, OrderType::Limit, "AAPL", 5000, 50);
    engine.processOrder(buy);

    ASSERT_EQ(trades.size(), 1);
    EXPECT_EQ(trades[0].sell_order_id, sA->id);
    EXPECT_EQ(sA->status, OrderStatus::Filled);
    EXPECT_EQ(sB->status, OrderStatus::New);
    EXPECT_EQ(engine.getBook("AAPL")->peekBestAsk()->id, sB->id);
}

TEST_F(MatchingEngineTest, Scenario8_MarketBuy) {
    auto* s1 = order_mgr.createOrder(Side::Sell, OrderType::Limit, "AAPL", 5000, 50);
    engine.processOrder(s1);
    auto* s2 = order_mgr.createOrder(Side::Sell, OrderType::Limit, "AAPL", 5100, 50);
    engine.processOrder(s2);

    auto* mkt_buy = order_mgr.createOrder(Side::Buy, OrderType::Market, "AAPL", 0, 80);
    engine.processOrder(mkt_buy);

    ASSERT_EQ(trades.size(), 2);
    EXPECT_EQ(trades[0].quantity, 50);
    EXPECT_EQ(trades[0].price, 5000);
    EXPECT_EQ(trades[1].quantity, 30);
    EXPECT_EQ(trades[1].price, 5100);

    EXPECT_EQ(mkt_buy->status, OrderStatus::Filled);
    EXPECT_EQ(s2->remaining(), 20);
}

TEST_F(MatchingEngineTest, Scenario9_MarketBuyInsufficientLiquidity) {
    auto* s1 = order_mgr.createOrder(Side::Sell, OrderType::Limit, "AAPL", 5000, 30);
    engine.processOrder(s1);

    auto* mkt_buy = order_mgr.createOrder(Side::Buy, OrderType::Market, "AAPL", 0, 100);
    engine.processOrder(mkt_buy);

    ASSERT_EQ(trades.size(), 1);
    EXPECT_EQ(trades[0].quantity, 30);
    EXPECT_EQ(mkt_buy->filled_quantity, 30);
    EXPECT_EQ(mkt_buy->status, OrderStatus::PartiallyFilled);
    EXPECT_TRUE(engine.getBook("AAPL")->empty());
}

TEST_F(MatchingEngineTest, Scenario10_MarketSell) {
    auto* b1 = order_mgr.createOrder(Side::Buy, OrderType::Limit, "AAPL", 5200, 50);
    engine.processOrder(b1);
    auto* b2 = order_mgr.createOrder(Side::Buy, OrderType::Limit, "AAPL", 5100, 50);
    engine.processOrder(b2);

    auto* mkt_sell = order_mgr.createOrder(Side::Sell, OrderType::Market, "AAPL", 0, 70);
    engine.processOrder(mkt_sell);

    ASSERT_EQ(trades.size(), 2);
    EXPECT_EQ(trades[0].price, 5200);
    EXPECT_EQ(trades[0].quantity, 50);
    EXPECT_EQ(trades[1].price, 5100);
    EXPECT_EQ(trades[1].quantity, 20);
    EXPECT_EQ(mkt_sell->status, OrderStatus::Filled);
}

TEST_F(MatchingEngineTest, Scenario11_CancelRestingOrder) {
    auto* buy = order_mgr.createOrder(Side::Buy, OrderType::Limit, "AAPL", 5000, 100);
    engine.processOrder(buy);
    EXPECT_EQ(engine.getBook("AAPL")->totalOrderCount(), 1);

    EXPECT_TRUE(engine.cancelOrder(buy->id));
    EXPECT_EQ(buy->status, OrderStatus::Cancelled);
    EXPECT_EQ(engine.getBook("AAPL")->totalOrderCount(), 0);
}

TEST_F(MatchingEngineTest, Scenario12_CancelAlreadyFilledOrder) {
    auto* sell = order_mgr.createOrder(Side::Sell, OrderType::Limit, "AAPL", 5000, 100);
    engine.processOrder(sell);
    auto* buy = order_mgr.createOrder(Side::Buy, OrderType::Limit, "AAPL", 5000, 100);
    engine.processOrder(buy);

    EXPECT_FALSE(engine.cancelOrder(sell->id));
    EXPECT_FALSE(engine.cancelOrder(buy->id));
}

TEST_F(MatchingEngineTest, Scenario13_CancelNonExistentOrder) {
    EXPECT_FALSE(engine.cancelOrder(999999));
}

TEST_F(MatchingEngineTest, Scenario14_MultipleSymbols) {
    auto* apple_sell = order_mgr.createOrder(Side::Sell, OrderType::Limit, "AAPL", 5000, 100);
    engine.processOrder(apple_sell);

    auto* goog_buy = order_mgr.createOrder(Side::Buy, OrderType::Limit, "GOOG", 5000, 100);
    engine.processOrder(goog_buy);

    EXPECT_EQ(trades.size(), 0);
    EXPECT_EQ(engine.getBook("AAPL")->totalOrderCount(), 1);
    EXPECT_EQ(engine.getBook("GOOG")->totalOrderCount(), 1);
}

TEST_F(MatchingEngineTest, Scenario15_EmptyBookSubmission) {
    auto* buy = order_mgr.createOrder(Side::Buy, OrderType::Limit, "MSFT", 30000, 50);
    engine.processOrder(buy);

    EXPECT_EQ(trades.size(), 0);
    EXPECT_EQ(buy->status, OrderStatus::New);
    EXPECT_EQ(engine.getBook("MSFT")->totalOrderCount(), 1);
    EXPECT_EQ(*engine.getBook("MSFT")->bestBid(), 30000);
}

TEST_F(MatchingEngineTest, Scenario16_InvalidParameters) {
    engine.processOrder(nullptr);

    auto* invalid = order_mgr.createOrder(Side::Buy, OrderType::Limit, "AAPL", -10, 100);
    EXPECT_EQ(invalid, nullptr);
}
