#pragma once

#include "exchange/types.hpp"
#include "exchange/order.hpp"
#include "exchange/trade.hpp"
#include "exchange/order_book.hpp"
#include "exchange/order_manager.hpp"
#include "exchange/execution_reporter.hpp"

#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

namespace exchange {
class MatchingEngine {
public:
    MatchingEngine(OrderManager& order_manager, ExecutionReporter& reporter);
    ~MatchingEngine() = default;

    MatchingEngine(const MatchingEngine&) = delete;
    MatchingEngine& operator=(const MatchingEngine&) = delete;
    MatchingEngine(MatchingEngine&&) noexcept = default;
    MatchingEngine& operator=(MatchingEngine&&) noexcept = delete;

    void processOrder(Order* order);

    bool cancelOrder(OrderId id);

    OrderBook& getOrCreateBook(std::string_view symbol);
    [[nodiscard]] OrderBook* getBook(std::string_view symbol);
    [[nodiscard]] const OrderBook* getBook(std::string_view symbol) const;

    void clear();

private:
    OrderManager& order_manager_;
    ExecutionReporter& reporter_;

    std::unordered_map<std::string, std::unique_ptr<OrderBook>> books_;

    void matchBuyOrder(Order* incoming, OrderBook& book);
    void matchSellOrder(Order* incoming, OrderBook& book);
};
}
