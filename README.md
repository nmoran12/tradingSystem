# cpp-low-latency-orderbook

A C++20 **exchange simulator** that reconstructs an order book from market events, matches client orders with price-time priority, replays commands from CSV or **OBK1** binary files, measures latency on synthetic workloads, and optionally streams replay steps to a small React visualiser.

This is an **educational / portfolio** project: no live markets, no brokerage connectivity, no trading bot.

| | |
|--|--|
| **Tests** | **164/164** via `./scripts/verify.sh` |
| **Queue** | **9A** performance deep dive **CURRENT** · **8C** done ([MILESTONE_8C_DECISION.md](docs/MILESTONE_8C_DECISION.md)) · **8A–8B** done |
| **CI** | GitHub Actions — build + **164** tests + UI build ([`.github/workflows/ci.yml`](.github/workflows/ci.yml)) |

Run all commands from **`cpp-low-latency-orderbook/`** (this directory), not the parent workspace folder.

---

## What I built

A single-threaded, deterministic core with clear module boundaries:

| Layer | What it does |
|-------|----------------|
| **Order book** | Resting liquidity, FIFO at each price, best bid/ask, cancel/execute replay |
| **Matching engine** | Limit/market orders, cancel, modify; emits `EngineEvent`s (trades, rejects, rests) |
| **CSV paths** | Market-event replay → book; command CSV → engine |
| **OBK1 binary** | 64-byte wire messages; `.obk` file read/write; `--binary-engine` CLI |
| **Benchmarks** | `matching_engine_benchmark`, `binary_protocol_benchmark`, `ring_buffer_pipeline_benchmark` with documented Release workflow |
| **Profiling discipline** | Baseline tables and profiler notes — **machine-local samples**, not production SLA claims |
| **Optional visualiser** | NDJSON export, **localhost SSE** live stream, React UI with file/scenario load, live follow, and **run summary** metrics (informational only) |

Skills demonstrated: modern C++20, CMake + GoogleTest, protocol design, benchmark methodology, separation of hot path vs debug/demo tooling.

---

## Quick start (clone → build → test → demo)

```bash
git clone <your-repo-url>
cd cpp-low-latency-orderbook

cmake -S . -B build
cmake --build build
./scripts/verify.sh                    # 164 tests

# Headless demos
./build/cpp-low-latency-orderbook --engine data/sample_commands.csv
./build/cpp-low-latency-orderbook --replay data/sample_events.csv

# UI demo (offline — no .obk needed)
cd ui/replay-visualiser && npm install && npm run dev
# Open dev URL → load a bundled scenario (e.g. sample-replay)
```

**Full demo walkthrough** (binary file, NDJSON export, live SSE, fixture notes): **[docs/DEMO_GUIDE.md](docs/DEMO_GUIDE.md)**

**Requirements:** C++20, CMake 3.20+, Node.js/npm for the optional UI. First CMake configure may fetch GoogleTest over the network.

---

## Architecture at a glance

```text
REPLAY PATH
  market-events.csv → MarketDataParser → MarketEvent → OrderBook

ENGINE PATH (CSV)
  order-commands.csv → OrderCommandParser → MatchingEngine → EngineEvent → OrderBook

BINARY ENGINE PATH
  commands.obk (OBK1) → BinaryCommandReader → MatchingEngine → EngineEvent → OrderBook

OPTIONAL VISUALISATION (opt-in CLI on --binary-engine only)
  MatchingEngine steps → ReplayVisualisationWriter
    → NDJSON file (--export-visualisation)
    OR localhost SSE (--stream-visualisation)

OPTIONAL UI (separate npm app)
  bundled NDJSON / uploaded export / EventSource live stream → replay-visualiser
  run summary metrics computed client-side (not benchmark throughput)

BENCHMARKS (separate binaries; not default CLI)
  WorkloadGenerator → engine / binary phases / optional SPSC pipeline
```

Deeper module rules: [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md).

**Planned systems work (not implemented):** TCP order gateway, market data publisher, persistence/replay log — [docs/ROADMAP.md](docs/ROADMAP.md).

---

## Build and test

```bash
cmake -S . -B build
cmake --build build
./scripts/verify.sh
```

```bash
cd ui/replay-visualiser
npm install
npm run build    # optional; npm run dev for interactive demo
```

CI runs on push/PR via [`.github/workflows/ci.yml`](.github/workflows/ci.yml) (Ubuntu: CMake build, `ctest`, optional UI `npm run build`). Local `./scripts/verify.sh` remains the same check developers run before pushing.

---

## Run (headless)

| Mode | Command |
|------|---------|
| Market replay | `./build/cpp-low-latency-orderbook --replay data/sample_events.csv` |
| Command engine | `./build/cpp-low-latency-orderbook --engine data/sample_commands.csv` |
| Binary engine | `./build/cpp-low-latency-orderbook --binary-engine /path/to/commands.obk` |

**Generate a local `.obk`** (not committed):

```bash
./build/binary_protocol_benchmark 5000 42
# Use the "Binary file:" path printed in the output
```

Protocol layout: [docs/BINARY_PROTOCOL.md](docs/BINARY_PROTOCOL.md).

---

## Replay visualiser

React + Vite under `ui/replay-visualiser/`:

| Feature | Description |
|---------|-------------|
| **File / scenarios** | Upload CLI-exported NDJSON or load bundled `public/sample-*.ndjson` |
| **Live SSE** | `EventSource` to `--stream-visualisation` (start C++ first, then Connect in UI) |
| **Run summary** | Totals, trade stats, final BBO, min/max spread — **not** Release benchmark numbers |

```bash
# Export then view in UI
./build/cpp-low-latency-orderbook \
  --binary-engine /path/to/commands.obk \
  --export-visualisation /tmp/replay.ndjson

# Live stream (see docs/DEMO_GUIDE.md for two-terminal flow)
./build/cpp-low-latency-orderbook \
  --binary-engine /path/to/commands.obk \
  --stream-visualisation 127.0.0.1:9000
```

Bundled scenarios may show **richer depth** than the C++ exporter (BBO-only). See [docs/REPLAY_VISUALISER.md](docs/REPLAY_VISUALISER.md).

---

## Benchmarks and profiling

```bash
./build/matching_engine_benchmark 100000 42
./build/binary_protocol_benchmark 100000 42
./build/ring_buffer_pipeline_benchmark 100000 42
./scripts/benchmark_release.sh 100000 42
./scripts/benchmark_repeat.sh 5 100000 42   # if present — repeated runs for stability
```

| Document | Purpose |
|----------|---------|
| [docs/BENCHMARKING.md](docs/BENCHMARKING.md) | Harness, workloads, what each metric means |
| [docs/PERFORMANCE_BASELINE.md](docs/PERFORMANCE_BASELINE.md) | **Dated** local Release samples — compare on your machine only |
| [docs/PROFILING_REPORT.md](docs/PROFILING_REPORT.md) | Instruments workflow, stability notes |

**Do not** treat UI run summary or a single benchmark run as proof of production latency. Repeat Release builds and report typical (e.g. median) results when claiming improvements.

---

## README media (optional)

Screenshot/GIF assets are **not** in the repository yet. When you capture them, follow the checklist in [docs/DEMO_GUIDE.md](docs/DEMO_GUIDE.md) and link from here (e.g. `docs/images/demo-offline.png`).

---

## Repository hygiene

- **`profiling/`** — local trace output; **keep untracked** (do not commit).
- **`build/`**, **`node_modules/`**, **`dist/`**, **`*.obk`** — local/generated; do not commit.
- **CI** — do not commit secrets; workflow is build/test only (no deploy).

---

## Documentation map

| Document | Description |
|----------|-------------|
| [docs/DEMO_GUIDE.md](docs/DEMO_GUIDE.md) | **Demo commands** — clone through live SSE |
| [docs/PROJECT_OVERVIEW.md](docs/PROJECT_OVERVIEW.md) | Goals, components, portfolio framing |
| [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) | Module boundaries and invariants |
| [docs/ROADMAP.md](docs/ROADMAP.md) | Completed milestones and systems candidates |
| [docs/FUTURE_IMPROVEMENTS.md](docs/FUTURE_IMPROVEMENTS.md) | Backlog (CI, tests, next features) |
| [docs/MILESTONE_QUEUE.md](docs/MILESTONE_QUEUE.md) | Delivery queue |
| [docs/MILESTONE_8C_DECISION.md](docs/MILESTONE_8C_DECISION.md) | **Next direction:** **9A** perf plan; infrastructure deferred |
| [docs/ACTIVE_MILESTONE.md](docs/ACTIVE_MILESTONE.md) | Active milestone acceptance criteria |
| [docs/REPLAY_VISUALISER.md](docs/REPLAY_VISUALISER.md) | UI, export, stream, run summary |
| [docs/BINARY_PROTOCOL.md](docs/BINARY_PROTOCOL.md) | OBK1 layout |
| [docs/BENCHMARKING.md](docs/BENCHMARKING.md) | Benchmark methodology |

---

## Project layout

```text
include/                 Headers (market_data, order_book, matching_engine, protocol, viz, …)
src/                     Implementations + main.cpp
benchmarks/              Standalone benchmark binaries
ui/replay-visualiser/    Optional React replay UI
tests/                   GoogleTest (164 cases)
scripts/                 verify.sh, benchmark_release.sh, …
data/                    Sample CSV fixtures
docs/                    Architecture, roadmap, demo guide
```

---

## License

Educational / portfolio use. No warranty. Not for production trading.
