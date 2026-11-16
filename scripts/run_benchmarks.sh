#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"

if [[ ! -d "${BUILD_DIR}" ]]; then
    echo "Build directory not found. Configuring and building Release target..."
    cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE=Release
    cmake --build "${BUILD_DIR}" --parallel
fi

echo "=========================================================="
echo "          ExchangeCore Benchmark Suite Execution          "
echo "=========================================================="
echo "Date:      $(date)"
echo "Host:      $(uname -s) $(uname -m)"
if command -v sysctl >/dev/null 2>&1; then
    echo "CPU Model: $(sysctl -n machdep.cpu.brand_string 2>/dev/null || echo 'Apple Silicon')"
fi
echo "Compiler:  $(${CXX:-c++} --version 2>&1 | head -n 1)"
echo "=========================================================="
echo ""

echo ">>> [1/4] Running Order Insertion Benchmark..."
"${BUILD_DIR}/benchmarks/bench_order_insertion" --benchmark_format=console

echo ""
echo ">>> [2/4] Running Order Cancellation Benchmark..."
"${BUILD_DIR}/benchmarks/bench_order_cancellation" --benchmark_format=console

echo ""
echo ">>> [3/4] Running Core Matching Engine Benchmark..."
"${BUILD_DIR}/benchmarks/bench_matching" --benchmark_format=console

echo ""
echo ">>> [4/4] Running Full Pipeline Throughput Benchmark..."
"${BUILD_DIR}/benchmarks/bench_full_pipeline" --benchmark_format=console

echo ""
echo "=========================================================="
echo "All benchmarks completed successfully."
echo "=========================================================="
