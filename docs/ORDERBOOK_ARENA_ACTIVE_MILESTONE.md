# Active Milestone: A7 Website Challenge Browser and Prompt Pages

**Status:** Complete

## Previous Milestone

A6, Replay Export and Replay Visualiser, was committed as `9d161bb` with commit
message:

```text
feat: add arena replay visualiser
```

## Goal

Establish the website as the main surface for finding and understanding Arena
challenges while execution remains local.

## Implemented Scope

- [x] Challenge browser at `#/challenges`.
- [x] Beat the Market Order detail page at
  `#/challenges/beat_market_order`.
- [x] Difficulty, tags, challenge type, description, and language display.
- [x] Prompt, objective, market settings, actions, scoring, and result metrics.
- [x] C++ starter matching the existing local process interface.
- [x] Python callback shown as contract-only, not executable.
- [x] Exact built-in and C++ local evaluator commands.
- [x] Result/replay generation and replay import workflow.
- [x] A6 replay visualiser preserved at `#/replay`.
- [x] Challenge content imported directly from the versioned JSON definition.
- [x] Local-only status and current limitations visible throughout the site.
- [x] No hosted Run or Submit controls.
- [x] Production build, TypeScript, route, content, and scope checks.

## Data Boundary

The frontend imports `arena/challenges/beat_market_order.v1.json` at build
time. Challenge facts are not maintained in a separate frontend metadata file.

Frontend-owned prose explains the current workflow and implementation status.
It does not redefine challenge rules.

## Architecture Boundary

A7 does not add hosted execution, sandboxing, accounts, leaderboards, online
submission, new challenge types, external Python execution, or C++ matching
engine integration.

The website remains a static local frontend.

## Checks

```bash
python3 -m json.tool arena/challenges/beat_market_order.v1.json
python3 -m unittest discover -s arena/tests -p 'test_*.py' -v
cmake -S arena/cpp -B build/arena-cpp
cmake --build build/arena-cpp
python3 arena/tools/evaluate_execution_v1.py \
  --challenge arena/challenges/beat_market_order.v1.json \
  --strategy simple_reference \
  --results-out arena/results/builtin.result.json \
  --replay-out arena/replays/builtin.replay.jsonl
python3 arena/tools/evaluate_execution_v1.py \
  --challenge arena/challenges/beat_market_order.v1.json \
  --strategy-process ./build/arena-cpp/arena_simple_reference_strategy \
  --results-out arena/results/cpp.result.json \
  --replay-out arena/replays/cpp.replay.jsonl
cd ui/replay-visualiser
npx tsc --noEmit
npm test
cd ../..
./scripts/verify.sh
git diff --check
```

## Exit Condition

A7 is committed separately. Result-file import, online editors, strategy
execution, accounts, leaderboards, and hosted judging remain outside this
milestone.
