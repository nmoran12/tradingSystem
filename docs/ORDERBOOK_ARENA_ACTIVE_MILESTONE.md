# Active Milestone: A3 Local `execution_v1` Evaluator Skeleton

**Status:** Complete

## Previous Milestone

A2, Challenge Definition Schema, was committed as `e5074ab` with commit
message:

```text
docs: define beat market order challenge schema
```

## Goal

Prove the deterministic local evaluation loop for **Beat the Market Order**
before adding external Python or C++ strategy execution.

A3 loads the `execution_v1` challenge, generates deterministic episodes,
evaluates built-in strategies, compares them with the immediate-market
baseline, and writes result JSON and replay JSONL.

## Implemented Scope

- [x] Challenge config loading and focused validation.
- [x] Explicit SplitMix64 PRNG with unsigned 64-bit arithmetic.
- [x] Deterministic single-symbol level-book episode generation.
- [x] Immediate market-order baseline.
- [x] Simple limit-then-market-cleanup reference strategy.
- [x] Strict action validation: any invalid action invalidates the episode.
- [x] Full-completion requirement with zero score otherwise.
- [x] Aggregate scoring across every local evaluation seed.
- [x] Versioned result JSON and replay JSONL output.
- [x] Focused standard-library tests.
- [x] Minimal evaluator run documentation.

## Boundaries

A3 does not:

- execute user-provided Python or C++;
- provide process isolation, timeouts, or sandboxing;
- invoke the C++ matching engine;
- implement networking, historical data, multiple assets, accounts, UI, or
  leaderboards.

The result and replay metadata identify the simulation as
`python_level_book_skeleton_v1` and state that the C++ matching engine is not
used. A later milestone must replace or adapt this model without silently
claiming parity.

## Acceptance Criteria

- the example challenge loads and validates;
- baseline and reference strategies are deterministic;
- every configured local evaluation seed contributes to the aggregate;
- invalid actions produce an invalid, incomplete, zero-score episode;
- incomplete episodes receive zero;
- the CLI writes valid result JSON and replay JSONL;
- generated artifacts are ignored by Git;
- tests and repository checks pass.

## Checks

```bash
python3 -m json.tool arena/challenges/beat_market_order.v1.json
python3 -m unittest discover -s arena/tests -p 'test_*.py' -v
python3 arena/tools/evaluate_execution_v1.py \
  --challenge arena/challenges/beat_market_order.v1.json \
  --strategy simple_reference \
  --results-out arena/results/beat_market_order.sample.result.json \
  --replay-out arena/replays/beat_market_order.sample.replay.jsonl
git diff --check
```

## Exit Condition

A3 is committed separately from A2. Do not start external Python strategy
execution or the C++ runner as part of this milestone.
