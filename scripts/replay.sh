#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"

INPUT_FILE="${1:-${ROOT_DIR}/data/sample_orders.jsonl}"
OUTPUT_EVENTS="${2:-${ROOT_DIR}/data/sample_events.jsonl}"

if [[ ! -f "${INPUT_FILE}" ]]; then
    echo "Input file ${INPUT_FILE} not found. Generating default sample data..."
    "${SCRIPT_DIR}/generate_market_data.sh" "${INPUT_FILE}" 10000 42
fi

if [[ ! -x "${BUILD_DIR}/exchange_main" ]]; then
    cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE=Release
    cmake --build "${BUILD_DIR}" --target exchange_main --parallel
fi

echo "Replaying ${INPUT_FILE} and logging engine events to ${OUTPUT_EVENTS}..."
"${BUILD_DIR}/exchange_main" --replay "${INPUT_FILE}" "${OUTPUT_EVENTS}"
