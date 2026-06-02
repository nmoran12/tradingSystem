#!/usr/bin/env bash
# Start the C++ live SSE replay stream server for the replay visualiser demo.
# Run the UI separately: cd ui/replay-visualiser && npm run dev
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${ROOT}"

BUILD_DIR="build"
COMMANDS=5000
SEED=42
STREAM_ADDR="127.0.0.1:9000"
FORCE=0

usage() {
    cat <<'EOF'
Usage: ./scripts/demo-live-replay.sh [options]

Writes a stable repo-local OBK1 file (not a temp benchmark artifact) and starts
the C++ stream server. Open the UI in another terminal.

Options:
  --force              Regenerate the .obk even if it already exists
  --commands N         Workload size (default: 5000)
  --seed N             Workload seed (default: 42)
  --build-dir DIR      CMake build directory (default: build)
  -h, --help           Show this help

UI (separate terminal):
  cd ui/replay-visualiser && npm install && npm run dev
  Connect to: http://127.0.0.1:9000/stream

Note: binary_protocol_benchmark writes to the system temp dir and deletes the
file when it finishes — do not use that path for demos.
EOF
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --force)
            FORCE=1
            shift
            ;;
        --commands)
            COMMANDS="$2"
            shift 2
            ;;
        --seed)
            SEED="$2"
            shift 2
            ;;
        --build-dir)
            BUILD_DIR="$2"
            shift 2
            ;;
        -h | --help)
            usage
            exit 0
            ;;
        *)
            echo "Unknown option: $1" >&2
            usage >&2
            exit 1
            ;;
    esac
done

OBK="${ROOT}/tmp/demo/live_demo_${COMMANDS}_${SEED}.obk"
CLI="${BUILD_DIR}/cpp-low-latency-orderbook"
WRITER="${BUILD_DIR}/write_demo_obk"

if [[ ! -x "${CLI}" || ! -x "${WRITER}" ]]; then
    echo "== Building demo targets (${BUILD_DIR}/) =="
    cmake -S . -B "${BUILD_DIR}"
    cmake --build "${BUILD_DIR}" --target cpp-low-latency-orderbook write_demo_obk
fi

if [[ ! -f "${OBK}" || "${FORCE}" -eq 1 ]]; then
    echo "== Writing demo OBK (${COMMANDS} commands, seed ${SEED}) =="
    "${WRITER}" "${OBK}" "${COMMANDS}" "${SEED}"
else
    echo "== Using existing demo OBK (${OBK}) =="
    echo "    (pass --force to regenerate)"
fi

STREAM_URL="http://${STREAM_ADDR}/stream"

cat <<EOF

== Live replay stream server ==
Binary file: ${OBK}
Stream URL:  ${STREAM_URL}

== Next: start the UI in another terminal ==
  cd ui/replay-visualiser
  npm install    # first time only
  npm run dev

Open the Vite dev URL, go to Live stream, paste:
  ${STREAM_URL}
then click Connect.

Debug without UI:
  curl -N ${STREAM_URL}

Press Ctrl+C here to stop the stream server.

EOF

exec "${CLI}" --binary-engine "${OBK}" --stream-visualisation "${STREAM_ADDR}"
