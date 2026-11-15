find_package(benchmark REQUIRED)

function(add_exchange_benchmark name source)
    add_executable(${name} ${source})
    target_link_libraries(${name} PRIVATE exchange_core benchmark::benchmark benchmark::benchmark_main)
endfunction()

add_exchange_benchmark(bench_order_insertion bench_order_insertion.cpp)
add_exchange_benchmark(bench_order_cancellation bench_order_cancellation.cpp)
add_exchange_benchmark(bench_matching bench_matching.cpp)
add_exchange_benchmark(bench_full_pipeline bench_full_pipeline.cpp)
