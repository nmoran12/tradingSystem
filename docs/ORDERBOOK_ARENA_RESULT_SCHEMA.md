# OrderBook Arena Result Schema

**Result schema version:** `1.1`

**Score version:** `1.0`

**Result generator:** `arena_execution_result` version `1.0`

## Scope

This document defines result JSON for the `execution_v1` challenge type. The
current implementation evaluates **Beat the Market Order** with
`python_level_book_skeleton_v1`.

The result explains how a strategy performed against the same-seed
`immediate_market_order_v1` baseline. It is not a statement about live-market
performance.

## Stability and Reproduction

Normal result files contain no wall-clock timestamp. The timestamp policy is
`none`.

For a fixed challenge file path, challenge contents, seed set, strategy
identifier, runner version, and deterministic strategy, result JSON is
byte-stable. The CLI records the requested replay output path, so changing that
path changes the result bytes without changing the evaluation.

Reproduction metadata records:

- challenge ID, challenge version, schema version, and source path;
- strategy mode, identifier, and API version;
- simulator, scenario generator, PRNG, runner, result-generator, and score
  versions;
- selected seed-set name and every evaluated seed.

## Top-Level Structure

| Field | Meaning |
|---|---|
| `schema_version` | Result contract version. Currently `1.1`. |
| `result_generator` | Generator name, version, and timestamp policy. |
| `runner_version` | Evaluator implementation version. |
| `challenge` | Challenge identity, version, type, title, and source path. |
| `strategy` | Built-in or external-process mode and strategy identifier. |
| `simulation` | Simulator model and deterministic generator details. |
| `reproduction` | Inputs and versions required to repeat the run. |
| `evaluation` | Seed list and valid, completed, incomplete, and invalid counts. |
| `scoring` | Score version, formulas, baseline, and aggregate score. |
| `aggregate_metrics` | Supporting metrics aggregated across all episodes. |
| `episodes` | Ordered per-seed results. |
| `replay` | Replay format, schema version, and CLI output path when present. |

`result_schema_version` is retained as an alias of `schema_version` for the A3
and A4 result consumers.

## Units and Metric Definitions

- **Quantity:** integer units from the challenge. The example uses a target of
  500 units.
- **Price:** integer ticks. A value of `10001` means 10,001 configured price
  ticks, not a currency amount.
- **`cash_spent_ticks`:** the integer sum of
  `fill_price_ticks * fill_quantity`. Its dimensional unit is tick-quantity.
  The existing field name is retained for compatibility.
- **`average_fill_price_ticks`:** `cash_spent_ticks / filled_quantity`, or
  `null` when there are no fills.
- **`fill_rate`:** `filled_quantity / target_quantity`, expressed as a ratio
  from `0.0` to `1.0`.
- **`arrival_midpoint_ticks`:** midpoint of the best bid and ask before the
  strategy's first action.
- **`slippage_ticks`:**
  `average_fill_price_ticks - arrival_midpoint_ticks`. For this buy task,
  lower is better and a negative value means execution below the arrival
  midpoint.
- **`slippage_bps`:**
  `slippage_ticks / arrival_midpoint_ticks * 10,000`.
- **`baseline_average_fill_price_ticks`:** VWAP of the immediate-market
  baseline on the same generated episode.
- **`improvement_ticks`:**
  `baseline_average_fill_price_ticks - average_fill_price_ticks`. Positive
  means the strategy bought more cheaply than the baseline.
- **`improvement_bps`:**
  `improvement_ticks / baseline_average_fill_price_ticks * 10,000`.

Floating-point metrics are rounded to six decimal places in result JSON.

## Episode Status and Score

An episode is:

- `completed` when it is valid and fills the full target;
- `incomplete` when it is valid but leaves quantity unfilled;
- `invalid` when an action, process response, or configured limit is invalid.

Score version `1.0` is:

```text
invalid episode   -> 0
incomplete episode -> 0
completed episode -> baseline_average_fill_price_ticks
                     - average_fill_price_ticks
```

A completed strategy that underperforms the baseline can receive a negative
score. This behavior is unchanged from A3/A4.

The aggregate score is the arithmetic mean of **all** episode scores in the
selected seed set. Invalid and incomplete zero scores remain in the mean.

## Per-Episode Fields

Each episode contains:

- seed, `status`, `valid`, `completed`, `invalid_reason`, and structured
  `error`;
- target, filled, and remaining quantity;
- fill rate, execution cost, strategy VWAP, baseline VWAP, slippage, and
  improvement;
- episode score;
- events processed, actions submitted, orders submitted, fills, and maximum
  simultaneous open orders;
- generator, PRNG, simulator, challenge, seed, and score reproduction data.

`fill_count` counts fill records, not filled quantity. `events_processed`
counts callbacks attempted, including a callback that produces an invalid
action or process response.

The older fields `completion_ratio`, `strategy_vwap_ticks`,
`baseline_vwap_ticks`, `strategy_slippage_ticks`,
`execution_cost_improvement_ticks`, `submitted_action_count`,
`submitted_order_count`, and `error_reason` remain compatibility aliases.
New consumers should use the canonical names above.

## Example

```json
{
  "schema_version": "1.1",
  "result_generator": {
    "name": "arena_execution_result",
    "timestamp_policy": "none",
    "version": "1.0"
  },
  "strategy": {
    "mode": "built_in",
    "identifier": "simple_reference_limit_then_market_cleanup"
  },
  "scoring": {
    "score_version": "1.0",
    "aggregate_score": 1.865333
  },
  "aggregate_metrics": {
    "episode_count": 3,
    "completed_episode_count": 3,
    "mean_fill_rate": 1.0,
    "score": 1.865333
  },
  "episodes": [
    {
      "seed": 901,
      "status": "completed",
      "filled_quantity": 500,
      "average_fill_price_ticks": 9999.804,
      "baseline_average_fill_price_ticks": 10001.8,
      "improvement_ticks": 1.996,
      "score": 1.996
    }
  ]
}
```

The example omits fields for readability.

## Reproduce a Result

Built-in reference:

```bash
python3 arena/tools/evaluate_execution_v1.py \
  --challenge arena/challenges/beat_market_order.v1.json \
  --strategy simple_reference \
  --results-out arena/results/builtin.result.json \
  --replay-out arena/replays/builtin.replay.jsonl
```

C++ reference:

```bash
cmake -S arena/cpp -B build/arena-cpp
cmake --build build/arena-cpp

python3 arena/tools/evaluate_execution_v1.py \
  --challenge arena/challenges/beat_market_order.v1.json \
  --strategy-process ./build/arena-cpp/arena_simple_reference_strategy \
  --results-out arena/results/cpp.result.json \
  --replay-out arena/replays/cpp.replay.jsonl
```

## Current Limitations

- The simulator is a deterministic Python level-book skeleton, not the
  repository's C++ matching engine.
- Scenarios are synthetic and the local evaluation seeds are inspectable.
- The result does not include trading PnL, drawdown, inventory risk, or
  real-market fees because those concepts are not defined for this buy-only
  completion task.
- The external C++ process is trusted local execution, not a sandbox.
- Result JSON does not prove strategy provenance and must not be treated as a
  verified leaderboard submission.
