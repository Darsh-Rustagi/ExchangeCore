#pragma once

#include "exchange/types.hpp"
#include "exchange/order.hpp"

#include <list>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace exchange {
struct PriceLevel {
    Price       price{0};
    Quantity    total_quantity{0};
    std::size_t order_count{0};
};

class OrderBook {
public:
    explicit OrderBook(std::string_view symbol);
    ~OrderBook() = default;

    OrderBook(const OrderBook&) = delete;
    OrderBook& operator=(const OrderBook&) = delete;
    OrderBook(OrderBook&&) noexcept = default;
    OrderBook& operator=(OrderBook&&) noexcept = default;

    [[nodiscard]] std::string_view symbol() const noexcept { return symbol_; }

    void addOrder(Order* order);

    bool removeOrder(OrderId id);

    [[nodiscard]] std::optional<Price> bestBid() const;
    [[nodiscard]] std::optional<Price> bestAsk() const;

    [[nodiscard]] Order* peekBestBid();
    [[nodiscard]] Order* peekBestAsk();

    [[nodiscard]] std::vector<PriceLevel> getBidDepth(std::size_t max_levels = 10) const;
    [[nodiscard]] std::vector<PriceLevel> getAskDepth(std::size_t max_levels = 10) const;

    [[nodiscard]] std::optional<Price> spread() const;

    [[nodiscard]] std::size_t bidLevelCount() const noexcept { return bids_.size(); }
    [[nodiscard]] std::size_t askLevelCount() const noexcept { return asks_.size(); }
    [[nodiscard]] std::size_t totalOrderCount() const noexcept { return order_locations_.size(); }
    [[nodiscard]] bool empty() const noexcept { return order_locations_.empty(); }

    void clear();

private:
    std::string symbol_;

    std::map<Price, std::list<Order*>, std::greater<Price>> bids_;

    std::map<Price, std::list<Order*>, std::less<Price>> asks_;

    struct OrderLocation {
        Side side;
        Price price;
        std::list<Order*>::iterator iterator;
    };
    std::unordered_map<OrderId, OrderLocation> order_locations_;
};
}
