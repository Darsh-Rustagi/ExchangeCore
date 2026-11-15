#include <benchmark/benchmark.h>
#include "exchange/order_book.hpp"
#include "exchange/order.hpp"

#include <algorithm>
#include <chrono>
#include <numeric>
#include <vector>

using namespace exchange;

static void BM_OrderCancellation(benchmark::State& state) {
    const std::size_t count = static_cast<std::size_t>(state.range(0));

    std::vector<Order> orders(count);
    OrderBook book("AAPL");

    for (std::size_t i = 0; i < count; ++i) {
        orders[i] = Order{
            .id = static_cast<OrderId>(i + 1),
            .symbol = "AAPL",
            .side = (i % 2 == 0) ? Side::Buy : Side::Sell,
            .type = OrderType::Limit,
            .price = static_cast<Price>(14000 + (i % 100) * 10),
            .quantity = 100,
            .filled_quantity = 0,
            .status = OrderStatus::New,
            .timestamp = 0
        };
        book.addOrder(&orders[i]);
    }

    std::vector<int64_t> latencies;
    latencies.reserve(count);

    std::size_t cancel_idx = 0;

    for (auto _ : state) {
        if (cancel_idx >= count) break;
        OrderId id_to_cancel = orders[cancel_idx++].id;

        auto t0 = std::chrono::high_resolution_clock::now();
        bool res = book.removeOrder(id_to_cancel);
        auto t1 = std::chrono::high_resolution_clock::now();

        auto duration_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
        latencies.push_back(duration_ns);

        benchmark::DoNotOptimize(res);
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

BENCHMARK(BM_OrderCancellation)
    ->Arg(50000)
    ->Iterations(50000);

BENCHMARK_MAIN();
