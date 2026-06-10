# OrderBook Arena Local Evaluator

## Status

The A3 evaluator is a deterministic Python skeleton for the first
`execution_v1` challenge. It proves challenge loading, seeded episode
generation, baseline comparison, scoring, and result/replay output.

It runs only built-in reference strategies. It does not execute user Python or
C++ code, provide sandboxing, call the C++ matching engine, or implement the
website. Those remain later milestones.

## Run

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

Generated result and replay files are ignored by Git. Their directories retain
`.gitkeep` files.

## Determinism

The scenario generator uses SplitMix64 implemented directly in
`evaluate_execution_v1.py`. It uses explicit unsigned 64-bit masking and modulo
selection. The evaluator does not use Python's `random` module.

For a fixed challenge file, seed set, runner version, and strategy, repeated
runs should produce byte-identical result and replay files.

## Built-In Strategies

- `baseline`: submits the full target as a market order on the first callback.
- `simple_reference`: rests one buy limit order inside the initial spread, then
  cancels it and buys any remainder at the final callback.

These strategies exercise the pipeline only. They are not claims of realistic
execution quality.

## Invalid and Incomplete Episodes

Any invalid action invalidates the episode. The result sets `completed` to
`false`, assigns a score of zero, and records an error reason.

An episode that reaches the end without filling the full target is incomplete
and also receives a score of zero.

## Replay Records

The JSONL replay includes episode metadata, market updates, visible book and
portfolio snapshots, strategy actions, fills, cancellations, errors, and final
metrics. Records include deterministic sequence numbers and episode seeds.

## Tests

```bash
python3 -m unittest discover -s arena/tests -p 'test_*.py' -v
```

The tests cover config loading, seed determinism, strict invalid-action
handling, full-completion scoring, local seed aggregation, and repeatability of
both built-in strategies.
