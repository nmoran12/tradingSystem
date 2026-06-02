#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${ROOT}"

RUN_COUNT="${1:-5}"
COMMAND_COUNT="${2:-100000}"
SEED="${3:-42}"

if ! [[ "${RUN_COUNT}" =~ ^[1-9][0-9]*$ ]]; then
    echo "Run count must be a positive integer (got: ${RUN_COUNT})" >&2
    exit 1
fi

if ! [[ "${COMMAND_COUNT}" =~ ^[1-9][0-9]*$ ]]; then
    echo "Command count must be a positive integer (got: ${COMMAND_COUNT})" >&2
    exit 1
fi

if ! [[ "${SEED}" =~ ^[0-9]+$ ]]; then
    echo "Seed must be a non-negative integer (got: ${SEED})" >&2
    exit 1
fi

echo "== Benchmark snapshot =="
echo "Runs:      ${RUN_COUNT}"
echo "Commands:  ${COMMAND_COUNT}"
echo "Seed:      ${SEED}"
echo
echo "Results are local and machine-dependent. Compare repeated Release runs on the same machine."
echo

echo "== Verify correctness before benchmarking =="
./scripts/verify.sh

echo
echo "== Record repeated Release benchmark snapshot =="
python3 ./scripts/record_benchmark_snapshot.py "${RUN_COUNT}" "${COMMAND_COUNT}" "${SEED}"
