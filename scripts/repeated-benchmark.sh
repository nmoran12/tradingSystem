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

OUT_DIR="benchmark-results"
TS="$(date +%Y%m%d-%H%M%S)"
RUN_DIR="${OUT_DIR}/${TS}-runs_${REPETITIONS}-count_${COMMAND_COUNT}-seed_${SEED}"
SUMMARY_FILE="${RUN_DIR}/summary.txt"

mkdir -p "${RUN_DIR}"

echo "== Repeated Release benchmarks =="
echo "Repetitions: ${REPETITIONS}"
echo "Commands:    ${COMMAND_COUNT}"
echo "Seed:        ${SEED}"
echo "Output dir:  ${RUN_DIR}"
echo
echo "Compare typical or median results across runs — not one lucky run."
echo

extract_ns_per_command() {
    local label="$1"
    local file="$2"
    # Extract the last "(NN ns/command)" for a given phase label.
    # Use BSD/GNU compatible grep+sed (avoid non-portable awk capture features).
    local value=""
    set +e
    value="$(
        grep -E "^[[:space:]]*${label}:" "${file}" 2>/dev/null \
            | sed -nE 's/.* ([0-9][0-9]*) ns\/command\).*/\1/p' \
            | tail -n 1
    )"
    set -e
    printf '%s' "${value}"
    return 0
}

write_summary() {
    local metric_name="$1"
    shift
    local values=("$@")

    if [[ "${#values[@]}" -eq 0 ]]; then
        echo "${metric_name}: (not found)" >> "${SUMMARY_FILE}"
        return
    fi

    printf '%s\n' "${values[@]}" | awk -v name="${metric_name}" '
        BEGIN { min = ""; max = ""; sum = 0; n = 0; }
        /^[0-9]+$/ {
            v = $1 + 0
            if (min == "" || v < min) min = v
            if (max == "" || v > max) max = v
            sum += v
            n += 1
        }
        END {
            if (n == 0) {
                printf("%s: (not found)\n", name)
            } else {
                avg = sum / n
                printf("%s: runs=%d min=%d max=%d avg=%.1f ns/command\n", name, n, min, max, avg)
            }
        }
    ' >> "${SUMMARY_FILE}"
}

declare -a binary_write_ns=()
declare -a buffered_read_decode_ns=()
declare -a buffered_engine_apply_ns=()
declare -a streaming_total_ns=()

for ((run = 1; run <= REPETITIONS; ++run)); do
    LOG_FILE="${RUN_DIR}/run_${run}.log"

    echo "========================================"
    echo "== Run ${run} of ${REPETITIONS} =="
    echo "== Log: ${LOG_FILE}"
    echo "========================================"

    ./scripts/benchmark_release.sh "${COMMAND_COUNT}" "${SEED}" | tee "${LOG_FILE}"

    bw="$(extract_ns_per_command "binary write" "${LOG_FILE}")"
    brd="$(extract_ns_per_command "buffered read/decode" "${LOG_FILE}")"
    bea="$(extract_ns_per_command "buffered engine apply" "${LOG_FILE}")"
    st="$(extract_ns_per_command "streaming read/decode/apply" "${LOG_FILE}")"

    [[ -n "${bw}" ]] && binary_write_ns+=("${bw}")
    [[ -n "${brd}" ]] && buffered_read_decode_ns+=("${brd}")
    [[ -n "${bea}" ]] && buffered_engine_apply_ns+=("${bea}")
    [[ -n "${st}" ]] && streaming_total_ns+=("${st}")

    echo
done

{
    echo "Repeated benchmark summary"
    echo "Timestamp:   ${TS}"
    echo "Repetitions: ${REPETITIONS}"
    echo "Commands:    ${COMMAND_COUNT}"
    echo "Seed:        ${SEED}"
    echo
    echo "Phase metrics are extracted from benchmark output when present (ns/command)."
    echo "Use this as a quick variance check; prefer median/typical values for claims."
    echo
} > "${SUMMARY_FILE}"

write_summary "binary write" ${binary_write_ns[@]+"${binary_write_ns[@]}"}
write_summary "buffered read/decode" ${buffered_read_decode_ns[@]+"${buffered_read_decode_ns[@]}"}
write_summary "buffered engine apply" ${buffered_engine_apply_ns[@]+"${buffered_engine_apply_ns[@]}"}
write_summary "streaming read/decode/apply" ${streaming_total_ns[@]+"${streaming_total_ns[@]}"}

echo "== All ${REPETITIONS} runs complete =="
echo "Summary: ${SUMMARY_FILE}"
echo "Review logs under: ${RUN_DIR}"
