#include "exchange/replay_engine.hpp"

#include <fstream>
#include <iostream>

namespace exchange {
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

ReplayEngine::ReplayEngine(OrderManager& order_manager,
                           MatchingEngine& matching_engine,
                           ExecutionReporter& reporter)
    : order_manager_(order_manager),
      matching_engine_(matching_engine),
      reporter_(reporter) {}

ReplayStats ReplayEngine::replayFromGenerator(MarketDataGenerator& generator) {
    ReplayStats stats;
    auto start_time = std::chrono::steady_clock::now();

    while (generator.hasNext()) {
        GeneratorEvent ev = generator.next();
        if (ev.action == GeneratorEvent::Action::NewOrder) {
            Order* ord = order_manager_.createOrder(
                ev.side, ev.type, ev.symbol, ev.price, ev.quantity, ev.timestamp, ev.order_id);
            if (ord) {
                matching_engine_.processOrder(ord);
                stats.orders_submitted++;
            }
        } else if (ev.action == GeneratorEvent::Action::CancelOrder) {
            if (matching_engine_.cancelOrder(ev.order_id)) {
                stats.orders_cancelled++;
            }
        }
    }

    auto end_time = std::chrono::steady_clock::now();
    stats.elapsed_wall_time = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    stats.trades_executed = reporter_.totalTrades();
    stats.shares_traded = reporter_.totalSharesTraded();
    stats.volume_value_cents = reporter_.totalVolumeValue();

    double seconds = static_cast<double>(stats.elapsed_wall_time.count()) / 1'000'000.0;
    if (seconds > 0.0) {
        stats.orders_per_second = static_cast<double>(stats.orders_submitted + stats.orders_cancelled) / seconds;
    }

    last_stats_ = stats;
    return stats;
}

ReplayStats ReplayEngine::replayFromFile(const std::filesystem::path& filepath) {
    ReplayStats stats;
    std::ifstream in(filepath);
    if (!in.is_open()) {
        return stats;
    }

    auto start_time = std::chrono::steady_clock::now();
    std::string line;

    while (std::getline(in, line)) {
        if (line.empty() || line.front() != '{') continue;

        std::string action = extractString(line, "action");
        OrderId order_id = static_cast<OrderId>(extractInt(line, "order_id"));
        std::string symbol = extractString(line, "symbol");
        Timestamp ts = static_cast<Timestamp>(extractInt(line, "timestamp"));

        if (action == "NewOrder") {
            Side side = sideFromString(extractString(line, "side"));
            OrderType type = orderTypeFromString(extractString(line, "type"));
            Price price = extractInt(line, "price");
            Quantity qty = static_cast<Quantity>(extractInt(line, "quantity"));

            Order* ord = order_manager_.createOrder(side, type, symbol, price, qty, ts, order_id);
            if (ord) {
                matching_engine_.processOrder(ord);
                stats.orders_submitted++;
            }
        } else if (action == "CancelOrder") {
            if (matching_engine_.cancelOrder(order_id)) {
                stats.orders_cancelled++;
            }
        }
    }

    auto end_time = std::chrono::steady_clock::now();
    stats.elapsed_wall_time = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    stats.trades_executed = reporter_.totalTrades();
    stats.shares_traded = reporter_.totalSharesTraded();
    stats.volume_value_cents = reporter_.totalVolumeValue();

    double seconds = static_cast<double>(stats.elapsed_wall_time.count()) / 1'000'000.0;
    if (seconds > 0.0) {
        stats.orders_per_second = static_cast<double>(stats.orders_submitted + stats.orders_cancelled) / seconds;
    }

    last_stats_ = stats;
    return stats;
}
}
