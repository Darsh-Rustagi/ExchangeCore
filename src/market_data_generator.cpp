#include "exchange/market_data_generator.hpp"

#include <cmath>
#include <fstream>
#include <sstream>

namespace exchange {
MarketDataGenerator::MarketDataGenerator(GeneratorConfig config)
    : config_(std::move(config)), rng_(config_.seed), next_order_id_(config_.start_order_id) {}

void MarketDataGenerator::reset() {
    rng_.seed(config_.seed);
    events_generated_ = 0;
    next_order_id_ = config_.start_order_id;
    current_sim_time_ = 1700000000000000000ULL;
    active_order_ids_.clear();
}

GeneratorEvent MarketDataGenerator::next() {
    events_generated_++;

    std::uniform_real_distribution<double> uniform_zero_one(0.0, 1.0);
    std::uniform_int_distribution<std::uint64_t> time_step_dist(1000, 50000);
    current_sim_time_ += time_step_dist(rng_);

    if (!active_order_ids_.empty() && uniform_zero_one(rng_) < config_.cancel_ratio) {
        std::uniform_int_distribution<std::size_t> idx_dist(0, active_order_ids_.size() - 1);
        std::size_t pick = idx_dist(rng_);
        OrderId target_id = active_order_ids_[pick];

        std::swap(active_order_ids_[pick], active_order_ids_.back());
        active_order_ids_.pop_back();

        GeneratorEvent ev{
            .action = GeneratorEvent::Action::CancelOrder,
            .order_id = target_id,
            .symbol = config_.symbol,
            .timestamp = current_sim_time_
        };
        return ev;
    }

    Side side = (uniform_zero_one(rng_) < 0.5) ? Side::Buy : Side::Sell;
    bool is_market = (uniform_zero_one(rng_) < config_.market_order_ratio);
    OrderType order_type = is_market ? OrderType::Market : OrderType::Limit;

    Price price = 0;
    if (!is_market) {
        std::normal_distribution<double> norm_dist(0.0, static_cast<double>(config_.price_std_dev));
        double offset = norm_dist(rng_);

        double skew = (side == Side::Buy) ? -static_cast<double>(config_.price_tick) : static_cast<double>(config_.price_tick);
        double raw_price = static_cast<double>(config_.base_price) + offset + skew;

        Price tick = std::max<Price>(1, config_.price_tick);
        price = static_cast<Price>(std::round(raw_price / static_cast<double>(tick))) * tick;
        if (price <= tick) {
            price = tick;
        }
    }

    std::uniform_int_distribution<Quantity> qty_dist(config_.min_quantity, config_.max_quantity);
    Quantity quantity = qty_dist(rng_);

    OrderId assigned_id = next_order_id_++;
    if (!is_market && active_order_ids_.size() < 10000) {
        active_order_ids_.push_back(assigned_id);
    }

    GeneratorEvent ev{
        .action = GeneratorEvent::Action::NewOrder,
        .order_id = assigned_id,
        .symbol = config_.symbol,
        .side = side,
        .type = order_type,
        .price = price,
        .quantity = quantity,
        .timestamp = current_sim_time_
    };
    return ev;
}

void MarketDataGenerator::generateToFile(const std::filesystem::path& filepath) {
    if (auto parent = filepath.parent_path(); !parent.empty()) {
        std::filesystem::create_directories(parent);
    }

    std::ofstream out(filepath, std::ios::out | std::ios::trunc);
    if (!out.is_open()) {
        return;
    }

    reset();
    while (hasNext()) {
        GeneratorEvent ev = next();
        std::ostringstream ss;
        if (ev.action == GeneratorEvent::Action::NewOrder) {
            ss << "{\"action\":\"NewOrder\","
               << "\"order_id\":" << ev.order_id << ","
               << "\"symbol\":\"" << ev.symbol << "\","
               << "\"side\":\"" << toString(ev.side) << "\","
               << "\"type\":\"" << toString(ev.type) << "\","
               << "\"price\":" << ev.price << ","
               << "\"quantity\":" << ev.quantity << ","
               << "\"timestamp\":" << ev.timestamp << "}";
        } else {
            ss << "{\"action\":\"CancelOrder\","
               << "\"order_id\":" << ev.order_id << ","
               << "\"symbol\":\"" << ev.symbol << "\","
               << "\"timestamp\":" << ev.timestamp << "}";
        }
        out << ss.str() << "\n";
    }
}
}
