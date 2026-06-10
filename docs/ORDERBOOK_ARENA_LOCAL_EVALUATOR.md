# OrderBook Arena Local Evaluator

## Status

The local evaluator is a deterministic Python simulation skeleton for the first
`execution_v1` challenge. It proves challenge loading, seeded episode
generation, strategy callbacks, baseline comparison, scoring, and
result/replay output.

It supports built-in evaluator strategies and a separately compiled C++
strategy process. It does not provide sandboxing, run external Python
strategies, call the C++ matching engine, or implement the website.

## Built-In Strategy

From the repository root:

```bash
python3 arena/tools/evaluate_execution_v1.py \
  --challenge arena/challenges/beat_market_order.v1.json \
  --strategy simple_reference \
  --results-out arena/results/beat_market_order.sample.result.json \
  --replay-out arena/replays/beat_market_order.sample.replay.jsonl
```

Use `--strategy baseline` to evaluate the immediate-market baseline. The
default seed set is `local_evaluation`; every seed in that set contributes to
the aggregate score.

## C++ Strategy Process

Build the included C++ example:

```bash
cmake -S arena/cpp -B build/arena-cpp
cmake --build build/arena-cpp
```

Then run it through the evaluator:

```bash
python3 arena/tools/evaluate_execution_v1.py \
  --challenge arena/challenges/beat_market_order.v1.json \
  --strategy-process ./build/arena-cpp/arena_simple_reference_strategy \
  --results-out arena/results/cpp.result.json \
  --replay-out arena/replays/cpp.replay.jsonl
```

The starter interface is
[`arena/cpp/execution_v1_strategy.hpp`](../arena/cpp/execution_v1_strategy.hpp).
User code implements:

```cpp
std::vector<Action> onBookUpdate(
    const BookView& book,
    const Portfolio& portfolio);
```

The header owns the stdin/stdout protocol loop. See
[`arena/protocol/execution_v1_protocol.md`](../arena/protocol/execution_v1_protocol.md)
for the JSON Lines contract and fixtures.

Generated result and replay files are ignored by Git. Their directories retain
`.gitkeep` files.

## Determinism

The scenario generator uses SplitMix64 implemented directly in
`evaluate_execution_v1.py`. It uses explicit unsigned 64-bit masking and modulo
selection. The evaluator does not use Python's `random` module.

For a fixed challenge file, seed set, runner version, and deterministic
strategy, repeated runs should produce byte-identical result and replay files
when the same output paths are used. Result JSON has no wall-clock timestamp.

## Built-In Strategies

- `baseline`: submits the full target as a market order on the first callback.
- `simple_reference`: rests one buy limit order inside the initial spread, then
  cancels it and buys any remainder at the final callback.

These strategies exercise the pipeline only. They are not claims of realistic
execution quality.

The C++ example implements the same limit-then-market-cleanup decisions as the
built-in `simple_reference` strategy.

## Process Boundary

The C++ executable is trusted local code. It runs with the current user's
permissions and is not isolated from the machine, filesystem, or network.

The evaluator applies a per-response timeout from
`limits.local_callback_timeout_ms`. This catches hangs for local development;
it is not a security control.

Invalid JSON, malformed response envelopes, process exit, timeout, and invalid
actions invalidate the current episode with score zero and a clear result
error.

The A4 C++ process supplies decisions only. Simulation still uses
`python_level_book_skeleton_v1`, and result metadata continues to state
`uses_cpp_matching_engine: false`.

## Invalid and Incomplete Episodes

Any invalid action invalidates the episode. The result sets `completed` to
`false`, assigns a score of zero, and records an error reason.

An episode that reaches the end without filling the full target is incomplete
and also receives a score of zero.

Completed episodes score the price-tick VWAP improvement over the same-seed
immediate-market baseline. The aggregate score is the arithmetic mean across
every selected seed, including zero scores from invalid or incomplete
episodes. See
[`ORDERBOOK_ARENA_RESULT_SCHEMA.md`](ORDERBOOK_ARENA_RESULT_SCHEMA.md) for
formulas, units, schema versions, and reproduction metadata.

## Replay Records

The JSONL replay includes episode metadata, market updates, visible book and
portfolio snapshots, strategy actions, fills, cancellations, errors, and final
metrics. Records include deterministic sequence numbers and episode seeds.

## Tests

```bash
python3 -m unittest discover -s arena/tests -p 'test_*.py' -v
```

The tests cover config loading, seed determinism, strict invalid-action
handling, full-completion scoring, local seed aggregation, C++ compilation,
protocol fixtures, C++ repeatability, invalid JSON, malformed responses,
process exit, timeout handling, hand-calculated metrics, byte stability, and
golden result fixtures.
