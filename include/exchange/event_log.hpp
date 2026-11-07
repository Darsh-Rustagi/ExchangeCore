#pragma once

#include "exchange/types.hpp"
#include "exchange/order.hpp"
#include "exchange/trade.hpp"

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace exchange {
struct Event {
    enum class Type {
        OrderAccepted,
        OrderRejected,
        TradeExecuted,
        OrderCancelled,
        OrderPartiallyFilled,
        OrderFilled
    };

    Type        type{Type::OrderAccepted};
    Timestamp   timestamp{0};
    OrderId     order_id{0};
    OrderId     counterparty_id{0};
    std::string symbol{};
    Side        side{Side::Buy};
    OrderType   order_type{OrderType::Limit};
    Price       price{0};
    Quantity    quantity{0};
    Quantity    remaining_quantity{0};
    std::string reason{};
};

class EventLog {
public:
    explicit EventLog(const std::filesystem::path& filepath);
    ~EventLog();

    EventLog(const EventLog&) = delete;
    EventLog& operator=(const EventLog&) = delete;
    EventLog(EventLog&&) noexcept;
    EventLog& operator=(EventLog&&) noexcept;

    void logOrderAccepted(const Order& order);
    void logOrderRejected(OrderId id, std::string_view symbol, std::string_view reason);
    void logTrade(const Trade& trade);
    void logOrderCancelled(OrderId id, std::string_view symbol, Quantity remaining);
    void logOrderPartiallyFilled(const Order& order, Quantity filled_in_this_step);
    void logOrderFilled(const Order& order);

    void flush();

    static std::vector<Event> loadEvents(const std::filesystem::path& filepath);

private:
    std::ofstream out_;
    std::filesystem::path filepath_;

    void writeJsonLine(const std::string& line);
};
}
