# Active Milestone: A6 Replay Export and Replay Visualiser

**Status:** Complete

## Previous Milestone

A5, Scoring Engine and Result JSON, was committed as `c224c21` with commit
message:

```text
feat: formalize arena scoring results
```

## Goal

Make deterministic Arena runs inspectable through a versioned replay artifact
and a lightweight local browser visualiser.

## Implemented Scope

- [x] Replay JSONL schema `1.0`.
- [x] Schema version, global record index, episode sequence, seed, event index,
  and record type on every record.
- [x] Explicit accepted and rejected action records.
- [x] Visible book, fills, portfolio updates, errors, and final episode state.
- [x] Result metadata for replay artifact name, record count, schema, and
  seeds.
- [x] Portable replay filename without absolute machine paths.
- [x] Deterministic built-in and C++ replay fixtures.
- [x] Replay validation, ordering, compatibility, and byte-stability tests.
- [x] Existing React/Vite visualiser adapted for Arena JSONL.
- [x] Local file loading, episode/event controls, book, actions, fills,
  portfolio, summary metrics, and useful validation errors.
- [x] Clear UI statement that no hosted Run or Submit behavior exists.

## Architecture Boundary

A6 still uses `python_level_book_skeleton_v1`. It does not invoke the C++
matching engine or add challenge browsing, external Python execution, hosted
execution, sandboxing, accounts, or leaderboards.

The visualiser reads local files in the browser. It has no backend.

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
cd ui/replay-visualiser && npm ci && npm run build
./scripts/verify.sh
git diff --check
```

## Exit Condition

A6 is committed separately. Challenge browsing pages, online strategy
execution, C++ matching-engine integration, and hosted judging remain outside
this milestone.
