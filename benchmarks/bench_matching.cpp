#include <benchmark/benchmark.h>
#include "exchange/types.hpp"
#include "exchange/order_manager.hpp"
#include "exchange/matching_engine.hpp"
#include "exchange/execution_reporter.hpp"

#include <algorithm>
#include <chrono>
#include <numeric>
#include <vector>

using namespace exchange;

static void BM_SingleMatchExecution(benchmark::State& state) {
    OrderManager om;
    ExecutionReporter rep;
    MatchingEngine engine(om, rep);

    const std::size_t count = 100000;
    for (std::size_t i = 1; i <= count; ++i) {
        auto* s = om.createOrder(Side::Sell, OrderType::Limit, "AAPL", 15000, 100, 0, i);
        engine.processOrder(s);
    }

    std::vector<int64_t> latencies;
    latencies.reserve(state.max_iterations);

    OrderId buy_id = count + 10;

    for (auto _ : state) {
        auto* buy = om.createOrder(Side::Buy, OrderType::Limit, "AAPL", 15000, 100, 0, buy_id++);

        auto t0 = std::chrono::high_resolution_clock::now();
        engine.processOrder(buy);
        auto t1 = std::chrono::high_resolution_clock::now();

        auto duration_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
        latencies.push_back(duration_ns);
    }

    if (!latencies.empty()) {
        std::sort(latencies.begin(), latencies.end());
        double avg = std::accumulate(latencies.begin(), latencies.end(), 0.0) / static_cast<double>(latencies.size());
        auto p50 = latencies[latencies.size() * 50 / 100];
        auto p95 = latencies[latencies.size() * 95 / 100];
        auto p99 = latencies[latencies.size() * 99 / 100];

        state.counters["avg_ns"] = avg;
        state.counters["p50_ns"] = static_cast<double>(p50);
        state.counters["p95_ns"] = static_cast<double>(p95);
        state.counters["p99_ns"] = static_cast<double>(p99);
    }

    state.SetItemsProcessed(latencies.size());
}

BENCHMARK(BM_SingleMatchExecution)->Iterations(50000);

BENCHMARK_MAIN();
