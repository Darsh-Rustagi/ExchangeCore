#include <gtest/gtest.h>
#include "exchange/types.hpp"
#include "exchange/order.hpp"

using namespace exchange;

TEST(OrderTest, DefaultConstructionAndRemaining) {
    Order order{
        .id = 101,
        .symbol = "AAPL",
        .side = Side::Buy,
        .type = OrderType::Limit,
        .price = 15000,
        .quantity = 250,
        .filled_quantity = 0,
        .status = OrderStatus::New,
        .timestamp = 1000
    };

    EXPECT_EQ(order.id, 101);
    EXPECT_EQ(order.symbol, "AAPL");
    EXPECT_EQ(order.remaining(), 250);
    EXPECT_FALSE(order.isFilled());

    order.filled_quantity = 100;
    EXPECT_EQ(order.remaining(), 150);
    EXPECT_FALSE(order.isFilled());

    order.filled_quantity = 250;
    EXPECT_EQ(order.remaining(), 0);
    EXPECT_TRUE(order.isFilled());

    order.filled_quantity = 300;
    EXPECT_EQ(order.remaining(), 0);
    EXPECT_TRUE(order.isFilled());
}

TEST(OrderTest, TypeStringConversions) {
    EXPECT_EQ(toString(Side::Buy), "Buy");
    EXPECT_EQ(toString(Side::Sell), "Sell");
    EXPECT_EQ(toString(OrderType::Limit), "Limit");
    EXPECT_EQ(toString(OrderType::Market), "Market");
    EXPECT_EQ(toString(OrderStatus::New), "New");
    EXPECT_EQ(toString(OrderStatus::Filled), "Filled");
    EXPECT_EQ(toString(OrderStatus::PartiallyFilled), "PartiallyFilled");
    EXPECT_EQ(toString(OrderStatus::Cancelled), "Cancelled");

    EXPECT_EQ(sideFromString("Buy"), Side::Buy);
    EXPECT_EQ(sideFromString("sell"), Side::Sell);
    EXPECT_EQ(orderTypeFromString("Market"), OrderType::Market);
    EXPECT_EQ(orderTypeFromString("Limit"), OrderType::Limit);
}
