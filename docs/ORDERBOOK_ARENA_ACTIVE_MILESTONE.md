# Active Milestone: A8 Website Result and Replay Viewer

**Status:** Complete

## Previous Milestone

A7, Website Challenge Browser and Prompt Pages, was committed as `2a0a983`:

```text
feat: add arena challenge browser
```

## Goal

Close the local MVP loop by letting users inspect evaluator result JSON and
its matching replay JSONL in the website.

## Implemented Scope

- [x] Result viewer at `#/results`.
- [x] Local result JSON import with result schema `1.1` validation.
- [x] Aggregate score, fill rate, improvement, identity, and version display.
- [x] Selectable per-episode metric table and invalid-reason display.
- [x] Replay artifact name, schema, record count, and seed display.
- [x] Matching replay JSONL import after a result is loaded.
- [x] Replay schema, record count, and ordered seed compatibility checks.
- [x] A6 replay inspector reused for the selected result episode.
- [x] Deterministic result/replay sample pair.
- [x] Focused parser, compatibility, route, content, and build checks.
- [x] Challenge browser and standalone replay routes preserved.
- [x] Local, unverified status visible with no hosted Run or Submit controls.

## Artifact Boundary

The browser reads files from the user's machine. It does not upload results,
verify how they were produced, execute strategy code, or grant leaderboard
eligibility.

Compatibility checks catch obvious accidental mismatches. They are not
cryptographic verification.

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

A8 is committed separately. Hosted execution, trusted result verification,
accounts, leaderboards, online submission, and C++ matching-engine integration
remain outside this milestone.
