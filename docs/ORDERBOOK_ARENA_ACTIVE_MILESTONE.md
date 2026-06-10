# Active Milestone: A4 Local C++ Strategy Runner

**Status:** Complete

## Previous Milestone

A3, Local `execution_v1` Evaluator Skeleton, was committed as `c45605e` with
commit message:

```text
feat: add deterministic execution evaluator skeleton
```

## Goal

Give trusted local C++ strategies the same `execution_v1` callback and action
semantics as built-in strategies through a separate compiled process.

The evaluator continues to own deterministic simulation, action validation,
portfolio state, baseline comparison, scoring, result generation, and replay
generation.

## Implemented Scope

- [x] Function-based C++ `BookView`, `Portfolio`, and `Action` interface.
- [x] Header-owned JSON Lines protocol loop.
- [x] Example C++ limit-then-market-cleanup strategy.
- [x] Standalone CMake build for the example.
- [x] One persistent trusted local process per evaluation.
- [x] `book_update`, `episode_end`, and `evaluation_end` messages.
- [x] Per-response timeout using the challenge's local timeout.
- [x] Protocol fixtures for normalized input and action output.
- [x] Result and replay output through the existing evaluator path.
- [x] Tests for build, success, determinism, fixtures, invalid JSON, malformed
  responses, early exit, and timeout.
- [x] Existing built-in strategy behavior and tests preserved.

## Security Boundary

This is trusted local execution only.

The strategy process:

- runs with the current user's permissions;
- is not sandboxed or isolated;
- may access local files, processes, and network resources;
- must not be exposed as hosted arbitrary-code execution.

The timeout is a development reliability limit, not a security boundary.

## Architecture Boundary

A4 does not invoke the C++ matching engine. The simulator remains
`python_level_book_skeleton_v1`, and result metadata states
`uses_cpp_matching_engine: false`.

A4 also does not add external Python execution, Docker, WASM, containers,
network services, website UI, accounts, leaderboards, or hosted execution.

## Checks

```bash
cmake -S arena/cpp -B build/arena-cpp
cmake --build build/arena-cpp
python3 -m unittest discover -s arena/tests -p 'test_*.py' -v
python3 arena/tools/evaluate_execution_v1.py \
  --challenge arena/challenges/beat_market_order.v1.json \
  --strategy-process ./build/arena-cpp/arena_simple_reference_strategy \
  --results-out arena/results/cpp.result.json \
  --replay-out arena/replays/cpp.replay.jsonl
./scripts/verify.sh
git diff --check
```

## Exit Condition

A4 is committed separately. Do not start C++ matching-engine integration,
external Python execution, or hosted execution as part of this milestone.
