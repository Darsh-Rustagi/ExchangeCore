#include <benchmark/benchmark.h>
#include "exchange/order_manager.hpp"
#include "exchange/matching_engine.hpp"
#include "exchange/execution_reporter.hpp"
#include "exchange/market_data_generator.hpp"
#include "exchange/replay_engine.hpp"

#include <chrono>
#include <vector>

using namespace exchange;

static void BM_FullPipelineThroughput(benchmark::State& state) {
    const std::size_t event_count = static_cast<std::size_t>(state.range(0));

    for (auto _ : state) {
        state.PauseTiming();
        GeneratorConfig cfg{
            .symbol = "AAPL",
            .base_price = 15000,
            .price_tick = 5,
            .price_std_dev = 100,
            .min_quantity = 10,
            .max_quantity = 500,
            .market_order_ratio = 0.10,
            .cancel_ratio = 0.15,
            .seed = 42,
            .total_events = event_count
        };
        MarketDataGenerator gen(cfg);

        OrderManager om;
        ExecutionReporter rep;
        MatchingEngine engine(om, rep);
        ReplayEngine replay(om, engine, rep);

        state.ResumeTiming();
        auto stats = replay.replayFromGenerator(gen);
        state.PauseTiming();

        state.counters["throughput_ops_sec"] = stats.orders_per_second;
        state.counters["trades_executed"] = static_cast<double>(stats.trades_executed);
        state.counters["shares_traded"] = static_cast<double>(stats.shares_traded);
        state.ResumeTiming();
    }

    state.SetItemsProcessed(event_count * state.iterations());
}

BENCHMARK(BM_FullPipelineThroughput)
    ->Arg(10000)
    ->Arg(50000)
    ->Unit(benchmark::kMillisecond)
    ->Iterations(5);

BENCHMARK_MAIN();
