#include <gtest/gtest.h>
#include "exchange/order_book.hpp"

using namespace exchange;

class OrderBookTest : public ::testing::Test {
protected:
    OrderBook book{"AAPL"};
};

TEST_F(OrderBookTest, EmptyBookInitialState) {
    EXPECT_EQ(book.symbol(), "AAPL");
    EXPECT_TRUE(book.empty());
    EXPECT_EQ(book.totalOrderCount(), 0);
    EXPECT_EQ(book.bidLevelCount(), 0);
    EXPECT_EQ(book.askLevelCount(), 0);
    EXPECT_FALSE(book.bestBid().has_value());
    EXPECT_FALSE(book.bestAsk().has_value());
    EXPECT_EQ(book.peekBestBid(), nullptr);
    EXPECT_EQ(book.peekBestAsk(), nullptr);
    EXPECT_FALSE(book.spread().has_value());
}

TEST_F(OrderBookTest, AddAndQueryPriceLevels) {
    Order buy1{.id = 1, .symbol = "AAPL", .side = Side::Buy, .price = 10000, .quantity = 50};
    Order buy2{.id = 2, .symbol = "AAPL", .side = Side::Buy, .price = 10050, .quantity = 80};
    Order sell1{.id = 3, .symbol = "AAPL", .side = Side::Sell, .price = 10100, .quantity = 60};
    Order sell2{.id = 4, .symbol = "AAPL", .side = Side::Sell, .price = 10200, .quantity = 90};

    book.addOrder(&buy1);
    book.addOrder(&buy2);
    book.addOrder(&sell1);
    book.addOrder(&sell2);

    EXPECT_EQ(book.totalOrderCount(), 4);
    EXPECT_EQ(book.bidLevelCount(), 2);
    EXPECT_EQ(book.askLevelCount(), 2);

    ASSERT_TRUE(book.bestBid().has_value());
    EXPECT_EQ(*book.bestBid(), 10050);
    EXPECT_EQ(book.peekBestBid()->id, 2);

    ASSERT_TRUE(book.bestAsk().has_value());
    EXPECT_EQ(*book.bestAsk(), 10100);
    EXPECT_EQ(book.peekBestAsk()->id, 3);

    ASSERT_TRUE(book.spread().has_value());
    EXPECT_EQ(*book.spread(), 50);
}

TEST_F(OrderBookTest, TimePriorityAtSamePriceLevel) {
    Order buy1{.id = 10, .symbol = "AAPL", .side = Side::Buy, .price = 10000, .quantity = 50};
    Order buy2{.id = 11, .symbol = "AAPL", .side = Side::Buy, .price = 10000, .quantity = 75};

    book.addOrder(&buy1);
    book.addOrder(&buy2);

    EXPECT_EQ(book.peekBestBid()->id, 10);

    EXPECT_TRUE(book.removeOrder(10));
    EXPECT_EQ(book.peekBestBid()->id, 11);

    EXPECT_TRUE(book.removeOrder(11));
    EXPECT_EQ(book.peekBestBid(), nullptr);
    EXPECT_TRUE(book.empty());
}

TEST_F(OrderBookTest, DepthSnapshot) {
    Order b1{.id = 1, .symbol = "AAPL", .side = Side::Buy, .price = 10000, .quantity = 20};
    Order b2{.id = 2, .symbol = "AAPL", .side = Side::Buy, .price = 10000, .quantity = 30};
    Order b3{.id = 3, .symbol = "AAPL", .side = Side::Buy, .price = 9900,  .quantity = 100};

    book.addOrder(&b1);
    book.addOrder(&b2);
    book.addOrder(&b3);

    auto depth = book.getBidDepth(5);
    ASSERT_EQ(depth.size(), 2);
    EXPECT_EQ(depth[0].price, 10000);
    EXPECT_EQ(depth[0].total_quantity, 50);
    EXPECT_EQ(depth[0].order_count, 2);

    EXPECT_EQ(depth[1].price, 9900);
    EXPECT_EQ(depth[1].total_quantity, 100);
    EXPECT_EQ(depth[1].order_count, 1);
}

TEST_F(OrderBookTest, CancelNonExistentOrder) {
    EXPECT_FALSE(book.removeOrder(999999));
}
