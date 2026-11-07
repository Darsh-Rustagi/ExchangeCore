#include "exchange/event_log.hpp"

#include <iostream>
#include <sstream>

namespace exchange {
EventLog::EventLog(const std::filesystem::path& filepath)
    : filepath_(filepath) {
    if (auto parent = filepath.parent_path(); !parent.empty()) {
        std::filesystem::create_directories(parent);
    }
    out_.open(filepath, std::ios::out | std::ios::trunc);
}

EventLog::~EventLog() {
    flush();
    if (out_.is_open()) {
        out_.close();
    }
}

EventLog::EventLog(EventLog&& other) noexcept
    : out_(std::move(other.out_)), filepath_(std::move(other.filepath_)) {}

EventLog& EventLog::operator=(EventLog&& other) noexcept {
    if (this != &other) {
        flush();
        if (out_.is_open()) out_.close();
        out_ = std::move(other.out_);
        filepath_ = std::move(other.filepath_);
    }
    return *this;
}

void EventLog::writeJsonLine(const std::string& line) {
    if (out_.is_open()) {
        out_ << line << "\n";
    }
}

void EventLog::flush() {
    if (out_.is_open()) {
        out_.flush();
    }
}

void EventLog::logOrderAccepted(const Order& order) {
    std::ostringstream ss;
    ss << "{\"event\":\"OrderAccepted\","
       << "\"timestamp\":" << order.timestamp << ","
       << "\"order_id\":" << order.id << ","
       << "\"symbol\":\"" << order.symbol << "\","
       << "\"side\":\"" << toString(order.side) << "\","
       << "\"type\":\"" << toString(order.type) << "\","
       << "\"price\":" << order.price << ","
       << "\"quantity\":" << order.quantity << "}";
    writeJsonLine(ss.str());
}

void EventLog::logOrderRejected(OrderId id, std::string_view symbol, std::string_view reason) {
    std::ostringstream ss;
    ss << "{\"event\":\"OrderRejected\","
       << "\"timestamp\":" << currentNanos() << ","
       << "\"order_id\":" << id << ","
       << "\"symbol\":\"" << symbol << "\","
       << "\"reason\":\"" << reason << "\"}";
    writeJsonLine(ss.str());
}

void EventLog::logTrade(const Trade& trade) {
    std::ostringstream ss;
    ss << "{\"event\":\"TradeExecuted\","
       << "\"timestamp\":" << trade.timestamp << ","
       << "\"buy_id\":" << trade.buy_order_id << ","
       << "\"sell_id\":" << trade.sell_order_id << ","
       << "\"symbol\":\"" << trade.symbol << "\","
       << "\"price\":" << trade.price << ","
       << "\"quantity\":" << trade.quantity << "}";
    writeJsonLine(ss.str());
}

void EventLog::logOrderCancelled(OrderId id, std::string_view symbol, Quantity remaining) {
    std::ostringstream ss;
    ss << "{\"event\":\"OrderCancelled\","
       << "\"timestamp\":" << currentNanos() << ","
       << "\"order_id\":" << id << ","
       << "\"symbol\":\"" << symbol << "\","
       << "\"remaining\":" << remaining << "}";
    writeJsonLine(ss.str());
}

void EventLog::logOrderPartiallyFilled(const Order& order, Quantity filled_in_this_step) {
    std::ostringstream ss;
    ss << "{\"event\":\"OrderPartiallyFilled\","
       << "\"timestamp\":" << currentNanos() << ","
       << "\"order_id\":" << order.id << ","
       << "\"symbol\":\"" << order.symbol << "\","
       << "\"remaining\":" << order.remaining() << ","
       << "\"filled_step\":" << filled_in_this_step << "}";
    writeJsonLine(ss.str());
}

void EventLog::logOrderFilled(const Order& order) {
    std::ostringstream ss;
    ss << "{\"event\":\"OrderFilled\","
       << "\"timestamp\":" << currentNanos() << ","
       << "\"order_id\":" << order.id << ","
       << "\"symbol\":\"" << order.symbol << "\"}";
    writeJsonLine(ss.str());
}

namespace {
std::string extractString(const std::string& line, std::string_view key) {
    std::string needle = "\"" + std::string(key) + "\":\"";
    auto pos = line.find(needle);
    if (pos == std::string::npos) return "";
    pos += needle.size();
    auto end = line.find('"', pos);
    if (end == std::string::npos) return "";
    return line.substr(pos, end - pos);
}

int64_t extractInt(const std::string& line, std::string_view key, int64_t default_val = 0) {
    std::string needle = "\"" + std::string(key) + "\":";
    auto pos = line.find(needle);
    if (pos == std::string::npos) return default_val;
    pos += needle.size();
    try {
        size_t idx = 0;
        return std::stoll(line.substr(pos), &idx);
    } catch (...) {
        return default_val;
    }
}
}

std::vector<Event> EventLog::loadEvents(const std::filesystem::path& filepath) {
    std::vector<Event> events;
    std::ifstream in(filepath);
    if (!in.is_open()) {
        return events;
    }

    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line.front() != '{') continue;

        Event ev;
        std::string ev_type = extractString(line, "event");
        ev.timestamp = static_cast<Timestamp>(extractInt(line, "timestamp"));
        ev.symbol = extractString(line, "symbol");

        if (ev_type == "OrderAccepted") {
            ev.type = Event::Type::OrderAccepted;
            ev.order_id = static_cast<OrderId>(extractInt(line, "order_id"));
            ev.side = sideFromString(extractString(line, "side"));
            ev.order_type = orderTypeFromString(extractString(line, "type"));
            ev.price = extractInt(line, "price");
            ev.quantity = static_cast<Quantity>(extractInt(line, "quantity"));
        } else if (ev_type == "TradeExecuted") {
            ev.type = Event::Type::TradeExecuted;
            ev.order_id = static_cast<OrderId>(extractInt(line, "buy_id"));
            ev.counterparty_id = static_cast<OrderId>(extractInt(line, "sell_id"));
            ev.price = extractInt(line, "price");
            ev.quantity = static_cast<Quantity>(extractInt(line, "quantity"));
        } else if (ev_type == "OrderCancelled") {
            ev.type = Event::Type::OrderCancelled;
            ev.order_id = static_cast<OrderId>(extractInt(line, "order_id"));
            ev.remaining_quantity = static_cast<Quantity>(extractInt(line, "remaining"));
        } else if (ev_type == "OrderPartiallyFilled") {
            ev.type = Event::Type::OrderPartiallyFilled;
            ev.order_id = static_cast<OrderId>(extractInt(line, "order_id"));
            ev.remaining_quantity = static_cast<Quantity>(extractInt(line, "remaining"));
        } else if (ev_type == "OrderFilled") {
            ev.type = Event::Type::OrderFilled;
            ev.order_id = static_cast<OrderId>(extractInt(line, "order_id"));
        } else if (ev_type == "OrderRejected") {
            ev.type = Event::Type::OrderRejected;
            ev.order_id = static_cast<OrderId>(extractInt(line, "order_id"));
            ev.reason = extractString(line, "reason");
        } else {
            continue;
        }

        events.push_back(std::move(ev));
    }
    return events;
}
}
