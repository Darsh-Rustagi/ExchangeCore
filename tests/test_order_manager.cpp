#include <gtest/gtest.h>
#include "exchange/order_manager.hpp"

using namespace exchange;

class OrderManagerTest : public ::testing::Test {
protected:
    OrderManager mgr;
};

TEST_F(OrderManagerTest, ValidationRules) {
    EXPECT_EQ(mgr.createOrder(Side::Buy, OrderType::Limit, "", 100, 10), nullptr);

    EXPECT_EQ(mgr.createOrder(Side::Buy, OrderType::Limit, "AAPL", 100, 0), nullptr);

    EXPECT_EQ(mgr.createOrder(Side::Buy, OrderType::Limit, "AAPL", 0, 10), nullptr);
    EXPECT_EQ(mgr.createOrder(Side::Buy, OrderType::Limit, "AAPL", -50, 10), nullptr);

    auto* mkt = mgr.createOrder(Side::Buy, OrderType::Market, "AAPL", 0, 10);
    ASSERT_NE(mkt, nullptr);
    EXPECT_EQ(mkt->type, OrderType::Market);
    EXPECT_EQ(mkt->price, 0);
}

TEST_F(OrderManagerTest, MonotonicIdGeneration) {
    auto* o1 = mgr.createOrder(Side::Buy, OrderType::Limit, "AAPL", 100, 10);
    auto* o2 = mgr.createOrder(Side::Buy, OrderType::Limit, "AAPL", 100, 10);
    auto* o3 = mgr.createOrder(Side::Buy, OrderType::Limit, "AAPL", 100, 10);

    ASSERT_NE(o1, nullptr);
    ASSERT_NE(o2, nullptr);
    ASSERT_NE(o3, nullptr);

    EXPECT_EQ(o1->id, 1);
    EXPECT_EQ(o2->id, 2);
    EXPECT_EQ(o3->id, 3);
    EXPECT_EQ(mgr.totalOrders(), 3);
}

TEST_F(OrderManagerTest, CustomIdAndLookup) {
    auto* custom = mgr.createOrder(Side::Sell, OrderType::Limit, "AAPL", 200, 5, 0, 500);
    ASSERT_NE(custom, nullptr);
    EXPECT_EQ(custom->id, 500);

    EXPECT_TRUE(mgr.hasOrder(500));
    EXPECT_EQ(mgr.getOrder(500), custom);

    EXPECT_EQ(mgr.createOrder(Side::Sell, OrderType::Limit, "AAPL", 200, 5, 0, 500), nullptr);

    auto* next = mgr.createOrder(Side::Sell, OrderType::Limit, "AAPL", 200, 5);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(next->id, 501);
}

TEST_F(OrderManagerTest, ResetState) {
    mgr.createOrder(Side::Buy, OrderType::Limit, "AAPL", 100, 10);
    EXPECT_EQ(mgr.totalOrders(), 1);

    mgr.clear();
    EXPECT_EQ(mgr.totalOrders(), 0);

    auto* o = mgr.createOrder(Side::Buy, OrderType::Limit, "AAPL", 100, 10);
    EXPECT_EQ(o->id, 1);
}
