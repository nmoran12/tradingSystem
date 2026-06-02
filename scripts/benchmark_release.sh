#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${ROOT}"

COMMAND_COUNT="${1:-100000}"
SEED="${2:-42}"
BUILD_DIR="build-release"

echo "== Configure Release build =="
cmake -S . -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE=Release

echo
echo "== Build Release benchmark targets =="
cmake --build "${BUILD_DIR}" --target binary_protocol_benchmark matching_engine_benchmark

echo
echo "== Binary protocol benchmark (${COMMAND_COUNT} commands, seed ${SEED}) =="
if [[ ! -x "${BUILD_DIR}/binary_protocol_benchmark" ]]; then
    echo "Missing expected benchmark target: ${BUILD_DIR}/binary_protocol_benchmark" >&2
    exit 1
fi
"${BUILD_DIR}/binary_protocol_benchmark" "${COMMAND_COUNT}" "${SEED}"

echo
echo "== Matching engine benchmark (${COMMAND_COUNT} commands, seed ${SEED}) =="
if [[ ! -x "${BUILD_DIR}/matching_engine_benchmark" ]]; then
    echo "Missing expected benchmark target: ${BUILD_DIR}/matching_engine_benchmark" >&2
    exit 1
fi
ME_ARGS=("${COMMAND_COUNT}" "${SEED}")
if [[ -n "${MATCHING_ENGINE_EXTRA_ARGS:-}" ]]; then
    # shellcheck disable=SC2206
    ME_ARGS+=( ${MATCHING_ENGINE_EXTRA_ARGS} )
fi
"${BUILD_DIR}/matching_engine_benchmark" "${ME_ARGS[@]}"
