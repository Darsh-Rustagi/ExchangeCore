#pragma once

#include "exchange/types.hpp"

#include <filesystem>
#include <random>
#include <string>
#include <vector>

namespace exchange {
struct GeneratorConfig {
    std::string   symbol{"AAPL"};
    Price         base_price{15000};
    Price         price_tick{5};
    Price         price_std_dev{100};
    Quantity      min_quantity{10};
    Quantity      max_quantity{500};
    double        market_order_ratio{0.10};
    double        cancel_ratio{0.15};
    std::uint64_t seed{42};
    OrderId       start_order_id{1};
    std::size_t   total_events{10000};
};

struct GeneratorEvent {
    enum class Action {
        NewOrder,
        CancelOrder
    };

    Action      action{Action::NewOrder};
    OrderId     order_id{0};
    std::string symbol{};
    Side        side{Side::Buy};
    OrderType   type{OrderType::Limit};
    Price       price{0};
    Quantity    quantity{0};
    Timestamp   timestamp{0};
};

class MarketDataGenerator {
public:
    explicit MarketDataGenerator(GeneratorConfig config);

    [[nodiscard]] bool hasNext() const noexcept {
        return events_generated_ < config_.total_events;
    }

    [[nodiscard]] std::size_t eventsRemaining() const noexcept {
        return (events_generated_ < config_.total_events) ? (config_.total_events - events_generated_) : 0;
    }

    GeneratorEvent next();

    void generateToFile(const std::filesystem::path& filepath);

    void reset();

    [[nodiscard]] const GeneratorConfig& config() const noexcept { return config_; }

private:
    GeneratorConfig config_;
    std::mt19937_64 rng_;
    std::size_t events_generated_{0};
    OrderId next_order_id_{1};
    Timestamp current_sim_time_{1700000000000000000ULL};

    std::vector<OrderId> active_order_ids_;
};
}
