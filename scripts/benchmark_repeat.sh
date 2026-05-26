#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${ROOT}"

REPETITIONS="${1:-5}"
COMMAND_COUNT="${2:-100000}"
SEED="${3:-42}"

if ! [[ "${REPETITIONS}" =~ ^[1-9][0-9]*$ ]]; then
    echo "Repetitions must be a positive integer (got: ${REPETITIONS})" >&2
    exit 1
fi

echo "== Repeated Release benchmarks =="
echo "Repetitions: ${REPETITIONS}"
echo "Commands:    ${COMMAND_COUNT}"
echo "Seed:        ${SEED}"
echo
echo "Compare typical or median results across runs — not one lucky run."
echo

for ((run = 1; run <= REPETITIONS; ++run)); do
    echo "========================================"
    echo "== Run ${run} of ${REPETITIONS} =="
    echo "========================================"
    ./scripts/benchmark_release.sh "${COMMAND_COUNT}" "${SEED}"
    echo
done

echo "== All ${REPETITIONS} runs complete =="
echo "Review the outputs above. Use median or typical phase metrics for before/after claims."
echo "Do not cite the fastest single run as a stable improvement."
