#include "exchange/execution_reporter.hpp"

namespace exchange {
ExecutionReporter::ExecutionReporter(EventLog* event_log)
    : event_log_(event_log) {}

void ExecutionReporter::onTrade(TradeCallback cb) {
    trade_callbacks_.push_back(std::move(cb));
}

void ExecutionReporter::onOrderAccepted(OrderCallback cb) {
    accepted_callbacks_.push_back(std::move(cb));
}

void ExecutionReporter::onOrderFilled(OrderCallback cb) {
    filled_callbacks_.push_back(std::move(cb));
}

void ExecutionReporter::onOrderPartiallyFilled(OrderCallback cb) {
    partial_callbacks_.push_back(std::move(cb));
}

void ExecutionReporter::onOrderCancelled(CancelCallback cb) {
    cancel_callbacks_.push_back(std::move(cb));
}

void ExecutionReporter::onOrderRejected(RejectCallback cb) {
    reject_callbacks_.push_back(std::move(cb));
}

void ExecutionReporter::reportOrderAccepted(const Order& order) {
    if (event_log_) {
        event_log_->logOrderAccepted(order);
    }
    for (const auto& cb : accepted_callbacks_) {
        cb(order);
    }
}

void ExecutionReporter::reportOrderRejected(OrderId id, std::string_view symbol, std::string_view reason) {
    if (event_log_) {
        event_log_->logOrderRejected(id, symbol, reason);
    }
    for (const auto& cb : reject_callbacks_) {
        cb(id, symbol, reason);
    }
}

void ExecutionReporter::reportTrade(const Trade& trade) {
    total_trades_++;
    total_shares_traded_ += trade.quantity;
    total_volume_value_ += static_cast<std::uint64_t>(trade.price) * trade.quantity;
    trade_history_.push_back(trade);

    if (event_log_) {
        event_log_->logTrade(trade);
    }
    for (const auto& cb : trade_callbacks_) {
        cb(trade);
    }
}

void ExecutionReporter::reportOrderPartiallyFilled(const Order& order, Quantity step_qty) {
    if (event_log_) {
        event_log_->logOrderPartiallyFilled(order, step_qty);
    }
    for (const auto& cb : partial_callbacks_) {
        cb(order);
    }
}

void ExecutionReporter::reportOrderFilled(const Order& order) {
    if (event_log_) {
        event_log_->logOrderFilled(order);
    }
    for (const auto& cb : filled_callbacks_) {
        cb(order);
    }
}

void ExecutionReporter::reportOrderCancelled(OrderId id, std::string_view symbol, Quantity remaining) {
    if (event_log_) {
        event_log_->logOrderCancelled(id, symbol, remaining);
    }
    for (const auto& cb : cancel_callbacks_) {
        cb(id, symbol, remaining);
    }
}

void ExecutionReporter::clear() {
    total_trades_ = 0;
    total_shares_traded_ = 0;
    total_volume_value_ = 0;
    trade_history_.clear();
}
}
