# Replay Visualiser

The browser visualiser under `ui/replay-visualiser/` inspects local OrderBook
Arena replay JSONL files. It is a React and Vite application with no backend.

It does not run strategies, submit results, connect to an exchange, or provide
hosted judging.

## Generate an Arena Replay

Built-in strategy:

```bash
python3 arena/tools/evaluate_execution_v1.py \
  --challenge arena/challenges/beat_market_order.v1.json \
  --strategy simple_reference \
  --results-out arena/results/builtin.result.json \
  --replay-out arena/replays/builtin.replay.jsonl
```

C++ strategy process:

```bash
cmake -S arena/cpp -B build/arena-cpp
cmake --build build/arena-cpp

python3 arena/tools/evaluate_execution_v1.py \
  --challenge arena/challenges/beat_market_order.v1.json \
  --strategy-process ./build/arena-cpp/arena_simple_reference_strategy \
  --results-out arena/results/cpp.result.json \
  --replay-out arena/replays/cpp.replay.jsonl
```

## Run the Visualiser

```bash
cd ui/replay-visualiser
npm ci
npm run dev
```

Open the local Vite URL and either:

- select **Load sample** for the committed built-in replay; or
- select **Open replay JSONL** and choose a CLI-generated file.

For a production build:

```bash
npm run build
```

The generated `dist/` directory is ignored by Git.

## Viewer Features

- local replay file import;
- useful errors for invalid, empty, incomplete, or unsupported files;
- episode selector for multi-seed evaluations;
- event slider with previous, next, and playback controls;
- visible bid and ask depth;
- accepted and rejected strategy actions;
- fills grouped by event;
- current portfolio and open orders;
- final status, score, fill, VWAP, and improvement summary;
- raw record types associated with the selected event.

The viewer does not show Run or Submit controls.

## Schema

Arena replay files use replay schema `1.0`. See
[`ORDERBOOK_ARENA_REPLAY_SCHEMA.md`](ORDERBOOK_ARENA_REPLAY_SCHEMA.md).

The older sample NDJSON files in `public/` describe the original C++ engine UI
spike and do not use the Arena schema. The Arena viewer rejects them rather
than guessing how to translate incompatible records.

## Limitations

- The current evaluator uses the Python simulator skeleton.
- Replay book depth is the strategy-visible depth, not full internal depth.
- The UI is a single local artifact page, not the later challenge website.
- Local replay files are unverified and cannot support a trusted leaderboard.
