#pragma once

#include "exchange/types.hpp"
#include "exchange/order_manager.hpp"
#include "exchange/matching_engine.hpp"
#include "exchange/execution_reporter.hpp"
#include "exchange/market_data_generator.hpp"

#include <chrono>
#include <filesystem>
#include <string>

namespace exchange {
struct ReplayStats {
    std::size_t               orders_submitted{0};
    std::size_t               orders_cancelled{0};
    std::size_t               trades_executed{0};
    Quantity                  shares_traded{0};
    std::uint64_t             volume_value_cents{0};
    std::chrono::microseconds elapsed_wall_time{0};
    double                    orders_per_second{0.0};
};

class ReplayEngine {
public:
    ReplayEngine(OrderManager& order_manager, MatchingEngine& matching_engine, ExecutionReporter& reporter);

    ReplayStats replayFromFile(const std::filesystem::path& filepath);

    ReplayStats replayFromGenerator(MarketDataGenerator& generator);

    [[nodiscard]] const ReplayStats& lastStats() const noexcept { return last_stats_; }

private:
    OrderManager&      order_manager_;
    MatchingEngine&    matching_engine_;
    ExecutionReporter& reporter_;
    ReplayStats        last_stats_;
};
}
