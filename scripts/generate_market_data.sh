#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"

OUTPUT_FILE="${1:-${ROOT_DIR}/data/sample_orders.jsonl}"
EVENT_COUNT="${2:-10000}"
SEED="${3:-42}"

mkdir -p "$(dirname "${OUTPUT_FILE}")"

if [[ ! -x "${BUILD_DIR}/exchange_main" ]]; then
    cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE=Release
    cmake --build "${BUILD_DIR}" --target exchange_main --parallel
fi

echo "Generating ${EVENT_COUNT} synthetic market events to ${OUTPUT_FILE} (seed=${SEED})..."
"${BUILD_DIR}/exchange_main" --generate "${OUTPUT_FILE}" "${EVENT_COUNT}" "${SEED}"
