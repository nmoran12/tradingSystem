#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${ROOT}"

RUN_COUNT=5
COMMAND_COUNT=100000
SEED=42
THROUGHPUT_ONLY=0
POSITIONAL=()

for arg in "$@"; do
    case "${arg}" in
        --throughput-only)
            THROUGHPUT_ONLY=1
            ;;
        *)
            POSITIONAL+=("${arg}")
            ;;
    esac
done

if [[ ${#POSITIONAL[@]} -ge 1 ]]; then
    RUN_COUNT="${POSITIONAL[0]}"
fi
if [[ ${#POSITIONAL[@]} -ge 2 ]]; then
    COMMAND_COUNT="${POSITIONAL[1]}"
fi
if [[ ${#POSITIONAL[@]} -ge 3 ]]; then
    SEED="${POSITIONAL[2]}"
fi

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
if [[ "${THROUGHPUT_ONLY}" -eq 1 ]]; then
    echo "Mode:      matching engine throughput-only"
else
    echo "Mode:      default (latency-sampling matching engine)"
fi
echo
echo "Results are local and machine-dependent. Compare repeated Release runs on the same machine."
echo

echo "== Verify correctness before benchmarking =="
./scripts/verify.sh

echo
echo "== Record repeated Release benchmark snapshot =="
PY_ARGS=("${RUN_COUNT}" "${COMMAND_COUNT}" "${SEED}")
if [[ "${THROUGHPUT_ONLY}" -eq 1 ]]; then
    PY_ARGS+=(--throughput-only)
fi
python3 ./scripts/record_benchmark_snapshot.py "${PY_ARGS[@]}"
