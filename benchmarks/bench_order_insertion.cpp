#include <benchmark/benchmark.h>
#include "exchange/order_book.hpp"
#include "exchange/order.hpp"

#include <algorithm>
#include <chrono>
#include <numeric>
#include <vector>

using namespace exchange;

static void BM_OrderInsertion(benchmark::State& state) {
    const std::size_t initial_orders = static_cast<std::size_t>(state.range(0));

    std::vector<Order> background_orders(initial_orders);
    for (std::size_t i = 0; i < initial_orders; ++i) {
        background_orders[i] = Order{
            .id = static_cast<OrderId>(i + 1),
            .symbol = "AAPL",
            .side = (i % 2 == 0) ? Side::Buy : Side::Sell,
            .type = OrderType::Limit,
            .price = static_cast<Price>(14000 + (i % 200) * 10),
            .quantity = 100,
            .filled_quantity = 0,
            .status = OrderStatus::New,
            .timestamp = 0
        };
    }

    OrderBook book("AAPL");
    for (auto& ord : background_orders) {
        book.addOrder(&ord);
    }

    std::vector<int64_t> latencies;
    latencies.reserve(100'000);

    OrderId next_id = initial_orders + 10;
    Price price_cursor = 14500;

    for (auto _ : state) {
        Order ord{
            .id = next_id++,
            .symbol = "AAPL",
            .side = Side::Buy,
            .type = OrderType::Limit,
            .price = price_cursor++,
            .quantity = 50,
            .filled_quantity = 0,
            .status = OrderStatus::New,
            .timestamp = 0
        };

        auto t0 = std::chrono::high_resolution_clock::now();
        book.addOrder(&ord);
        auto t1 = std::chrono::high_resolution_clock::now();

        auto duration_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
        latencies.push_back(duration_ns);

        benchmark::DoNotOptimize(book.totalOrderCount());
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

    state.SetItemsProcessed(state.iterations());
}

BENCHMARK(BM_OrderInsertion)
    ->Arg(100)
    ->Arg(1000)
    ->Arg(10000)
    ->Iterations(50000);

BENCHMARK_MAIN();
