# Active Milestone: A5 Scoring Engine and Result JSON

**Status:** Complete

## Previous Milestone

A4, Local C++ Strategy Runner, was committed as `b6873d8` with commit message:

```text
feat: add local c++ strategy runner
```

## Goal

Produce explainable, versioned result JSON from deterministic `execution_v1`
episodes without changing the existing strategy protocol or simulation model.

## Implemented Scope

- [x] Result schema `1.1` and result generator `1.0`.
- [x] Explicit no-wall-clock timestamp policy.
- [x] Score version `1.0` retained without behavior changes.
- [x] Canonical quantity, cost, VWAP, fill-rate, slippage, and improvement
  metrics with documented units and basis-point denominators.
- [x] Per-episode validity, completion, counters, errors, and reproduction
  metadata.
- [x] Aggregate counts, mean fill rate, mean completed improvement, and score.
- [x] Arithmetic-mean score across every configured local evaluation seed.
- [x] Hand-calculated unit tests for completed, partial, zero-fill, invalid,
  and incomplete cases.
- [x] Golden result fixtures for built-in, C++, invalid, and incomplete runs.
- [x] Byte-stability test for deterministic result serialization.
- [x] Existing built-in and C++ strategy-process behavior preserved.

## Score Contract

For a completed buy episode:

```text
score = baseline_average_fill_price_ticks - average_fill_price_ticks
```

Invalid and incomplete episodes score zero. A completed episode may have a
negative score when it performs worse than the baseline. The aggregate score
is the mean of all selected episode scores.

Score behavior remains version `1.0`. The result schema moved to `1.1` because
the result now formalizes metrics, counters, reproduction data, and error
fields.

## Architecture Boundary

A5 still uses `python_level_book_skeleton_v1`. It does not invoke the C++
matching engine, run external Python strategies, add website UI, or provide
hosted or sandboxed execution.

The C++ process remains trusted local code.

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
./scripts/verify.sh
git diff --check
```

## Exit Condition

A5 is committed separately. Replay schema/viewer work, C++ matching-engine
integration, external Python execution, and hosted execution remain outside
this milestone.
