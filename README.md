# cpp-low-latency-orderbook

A C++20 low-latency market data and order book engine that processes exchange-style events, reconstructs bid/ask depth, matches client orders, and measures processing latency. An optional React replay visualiser helps debug and demo behaviour without touching the benchmark hot path.

This project is an **educational / portfolio** exchange simulator: it does not connect to live markets and is not a trading bot.

**Status (post-7C):** Milestones **1–7C** complete on the ordered queue · **160/160** tests (`./scripts/verify.sh`) · binary OBK1 replay · Release benchmarks · optional replay UI (file, live SSE, run summary)

Run all commands from this directory (`cpp-low-latency-orderbook/`), not the parent workspace folder.

## What you get

- **Replay path** — market-event CSV → `OrderBook` (Milestone 1)
- **Engine path** — command CSV → `MatchingEngine` → trades and resting book (Milestones 2–3)
- **Binary engine** — OBK1 64-byte command files → `MatchingEngine` (Milestone 5)
- **Performance** — `matching_engine_benchmark`, `binary_protocol_benchmark`, `ring_buffer_pipeline_benchmark`, profiling docs (Milestones 4, 6)
- **Visualisation (optional)** — NDJSON export, localhost SSE stream, React UI with live follow and run-summary metrics (Milestones 7A–7C)

Benchmark numbers are **machine-dependent samples**, not production latency claims. See [docs/BENCHMARKING.md](docs/BENCHMARKING.md).

## Project documentation

| Document | Description |
|----------|-------------|
| [docs/PROJECT_OVERVIEW.md](docs/PROJECT_OVERVIEW.md) | Goals, components, portfolio value |
| [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) | Module boundaries, data flow, invariants |
| [docs/ROADMAP.md](docs/ROADMAP.md) | Completed milestones and long-term systems ideas |
| [docs/FUTURE_IMPROVEMENTS.md](docs/FUTURE_IMPROVEMENTS.md) | **Planned** backlog (CI, demo, tests, next milestones) |
| [docs/MILESTONE_QUEUE.md](docs/MILESTONE_QUEUE.md) | Ordered delivery queue (through 7C done) |
| [docs/DEVELOPMENT_RULES.md](docs/DEVELOPMENT_RULES.md) | Coding, testing, and architecture guardrails |
| [docs/BENCHMARKING.md](docs/BENCHMARKING.md) | Benchmark harness and methodology |
| [docs/PERFORMANCE_BASELINE.md](docs/PERFORMANCE_BASELINE.md) | Release-mode baseline workflow |
| [docs/PROFILING_REPORT.md](docs/PROFILING_REPORT.md) | Profiling tools and stability notes |
| [docs/REPLAY_VISUALISER.md](docs/REPLAY_VISUALISER.md) | Replay UI: file, live stream, run summary |

## Architecture at a glance

```text
REPLAY PATH
  market-events.csv → MarketDataParser → MarketEvent → OrderBook

ENGINE PATH
  order-commands.csv → OrderCommandParser → MatchingEngine → EngineEvent → OrderBook

BINARY ENGINE PATH
  commands.obk (OBK1) → BinaryCommandReader → MatchingEngine → EngineEvent → OrderBook

OPTIONAL VISUALISATION (opt-in CLI flags)
  MatchingEngine steps → ReplayVisualisationWriter → NDJSON file  OR  localhost SSE

OPTIONAL UI (separate app)
  NDJSON file / bundled scenarios / EventSource live stream → replay-visualiser

BENCHMARKS (separate binaries, not in default CLI)
  WorkloadGenerator → MatchingEngine / binary read / SPSC pipeline → metrics
```

Details: [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md).

## Build and verify

```bash
cmake -S . -B build
cmake --build build
./scripts/verify.sh          # build + ctest (160 tests)
```

Requirements: C++20, CMake 3.20+, network on first configure (GoogleTest via FetchContent).

**UI (optional):**

```bash
cd ui/replay-visualiser
npm install
npm run build
```

## Run

**Replay mode** (market-event CSV):

```bash
./build/cpp-low-latency-orderbook --replay data/sample_events.csv
```

**Engine mode** (command CSV):

```bash
./build/cpp-low-latency-orderbook --engine data/sample_commands.csv
```

**Binary engine** (OBK1 command file):

```bash
./build/cpp-low-latency-orderbook --binary-engine path/to/commands.obk
```

Generate local `.obk` files via `protocol::write_order_commands_binary` in tests/tooling (see [docs/BINARY_PROTOCOL.md](docs/BINARY_PROTOCOL.md) if present, or test helpers). `*.obk` may be gitignored.

**Visualisation export** (NDJSON, one JSON line per command step):

```bash
./build/cpp-low-latency-orderbook \
  --binary-engine path/to/commands.obk \
  --export-visualisation replay.ndjson
```

**Live visualisation stream** (localhost SSE, proof-of-concept):

```bash
./build/cpp-low-latency-orderbook \
  --binary-engine path/to/commands.obk \
  --stream-visualisation 127.0.0.1:9000
```

Then connect with the UI ([docs/REPLAY_VISUALISER.md](docs/REPLAY_VISUALISER.md)) or `curl -N http://127.0.0.1:9000/stream`.

## Benchmarks

```bash
./build/matching_engine_benchmark 100000 42
./build/binary_protocol_benchmark 100000 42
./build/ring_buffer_pipeline_benchmark 100000 42
./scripts/benchmark_release.sh 100000 42
./scripts/repeated-benchmark.sh   # see docs
```

See [docs/BENCHMARKING.md](docs/BENCHMARKING.md) and [docs/PERFORMANCE_BASELINE.md](docs/PERFORMANCE_BASELINE.md).

## Replay visualiser (7A–7C)

React + Vite app under `ui/replay-visualiser/`:

- Load bundled scenarios or uploaded NDJSON
- **Live follow** via `EventSource` against `--stream-visualisation`
- **Run summary** panel (client-side metrics; not Release benchmarks)

Full instructions: [docs/REPLAY_VISUALISER.md](docs/REPLAY_VISUALISER.md).

## Planned improvements (not implemented)

CI (GitHub Actions), demo script/screenshots, stronger equivalence tests, and the next systems milestone (TCP gateway vs persistence vs market data publisher) are **documented only**:

[docs/FUTURE_IMPROVEMENTS.md](docs/FUTURE_IMPROVEMENTS.md) · proposed queue **8A–8C** in [docs/MILESTONE_QUEUE.md](docs/MILESTONE_QUEUE.md)

## Project layout

```text
include/          Headers (market_data, order_book, matching_engine, protocol, viz, …)
src/              Implementations
benchmarks/       matching_engine, binary_protocol, ring_buffer_pipeline
ui/replay-visualiser/   Optional React UI
tests/            GoogleTest (160 cases)
scripts/          verify.sh, benchmark_release.sh, repeated-benchmark.sh
data/             sample CSV fixtures
docs/
```

## License

Educational / portfolio use. No warranty. Not for production trading.
