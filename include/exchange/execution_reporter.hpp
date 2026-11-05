#pragma once

#include "exchange/types.hpp"
#include "exchange/order.hpp"
#include "exchange/trade.hpp"
#include "exchange/event_log.hpp"

#include <functional>
#include <memory>
#include <vector>

namespace exchange {
class ExecutionReporter {
public:
    using TradeCallback = std::function<void(const Trade&)>;
    using OrderCallback = std::function<void(const Order&)>;
    using CancelCallback = std::function<void(OrderId, std::string_view, Quantity)>;
    using RejectCallback = std::function<void(OrderId, std::string_view, std::string_view)>;

    ExecutionReporter() = default;
    explicit ExecutionReporter(EventLog* event_log);

    void setEventLog(EventLog* event_log) noexcept { event_log_ = event_log; }

    void onTrade(TradeCallback cb);
    void onOrderAccepted(OrderCallback cb);
    void onOrderFilled(OrderCallback cb);
    void onOrderPartiallyFilled(OrderCallback cb);
    void onOrderCancelled(CancelCallback cb);
    void onOrderRejected(RejectCallback cb);

    void reportOrderAccepted(const Order& order);
    void reportOrderRejected(OrderId id, std::string_view symbol, std::string_view reason);
    void reportTrade(const Trade& trade);
    void reportOrderPartiallyFilled(const Order& order, Quantity step_qty);
    void reportOrderFilled(const Order& order);
    void reportOrderCancelled(OrderId id, std::string_view symbol, Quantity remaining);

    [[nodiscard]] std::size_t totalTrades() const noexcept { return total_trades_; }
    [[nodiscard]] Quantity totalSharesTraded() const noexcept { return total_shares_traded_; }
    [[nodiscard]] std::uint64_t totalVolumeValue() const noexcept { return total_volume_value_; }
    [[nodiscard]] const std::vector<Trade>& tradeHistory() const noexcept { return trade_history_; }

    void clear();

private:
    EventLog* event_log_{nullptr};

    std::vector<TradeCallback>  trade_callbacks_;
    std::vector<OrderCallback>  accepted_callbacks_;
    std::vector<OrderCallback>  filled_callbacks_;
    std::vector<OrderCallback>  partial_callbacks_;
    std::vector<CancelCallback> cancel_callbacks_;
    std::vector<RejectCallback> reject_callbacks_;

    std::size_t   total_trades_{0};
    Quantity      total_shares_traded_{0};
    std::uint64_t total_volume_value_{0};
    std::vector<Trade> trade_history_;
};
}
