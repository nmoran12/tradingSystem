#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${ROOT}"

step() {
  printf '\n==> %s\n' "$1"
}

step "1/7 Validate the Beat the Market Order challenge"
python3 -m json.tool \
  arena/challenges/beat_market_order.v1.json >/dev/null

step "2/7 Run Arena evaluator and artifact tests"
python3 -m unittest discover -s arena/tests -p 'test_*.py' -v

step "3/7 Build the trusted local C++ example strategy"
cmake -S arena/cpp -B build/arena-cpp
cmake --build build/arena-cpp

step "4/7 Generate built-in strategy result and replay artifacts"
python3 arena/tools/evaluate_execution_v1.py \
  --challenge arena/challenges/beat_market_order.v1.json \
  --strategy simple_reference \
  --results-out arena/results/builtin.result.json \
  --replay-out arena/replays/builtin.replay.jsonl

step "5/7 Generate Python strategy result and replay artifacts"
python3 arena/tools/evaluate_execution_v1.py \
  --challenge arena/challenges/beat_market_order.v1.json \
  --strategy-file arena/python/examples/simple_reference_strategy.py \
  --results-out arena/results/python.result.json \
  --replay-out arena/replays/python.replay.jsonl

step "6/7 Generate C++ strategy result and replay artifacts"
python3 arena/tools/evaluate_execution_v1.py \
  --challenge arena/challenges/beat_market_order.v1.json \
  --strategy-process ./build/arena-cpp/arena_simple_reference_strategy \
  --results-out arena/results/cpp.result.json \
  --replay-out arena/replays/cpp.replay.jsonl

step "7/7 Install locked frontend dependencies and build/test the website"
(
  cd ui/replay-visualiser
  npm ci
  npx tsc --noEmit
  npm test
)

printf '\nArena local MVP demo check passed.\n'
printf 'Generated artifacts:\n'
printf '  arena/results/builtin.result.json\n'
printf '  arena/replays/builtin.replay.jsonl\n'
printf '  arena/results/python.result.json\n'
printf '  arena/replays/python.replay.jsonl\n'
printf '  arena/results/cpp.result.json\n'
printf '  arena/replays/cpp.replay.jsonl\n'
printf 'Start the website with: cd ui/replay-visualiser && npm run dev\n'
