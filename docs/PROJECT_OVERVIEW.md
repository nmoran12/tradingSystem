# Project Overview

## cpp-low-latency-orderbook

A C++20 educational exchange simulator focused on market data, order-book reconstruction, deterministic order matching, binary command replay (OBK1), performance measurement, and an **optional** replay visualiser. The project separates **commands** (client input) from **events** (engine output), maintains price-time priority, and documents a path toward gateways and persistence—without live markets or real money.

## Problem it solves

Trading systems need a fast, correct core that can:

1. Maintain resting liquidity in price-time priority.
2. Match crossing orders and emit trade events.
3. Accept structured commands (new, cancel, modify) and reject invalid input.
4. Measure how long each operation takes on a given machine.
5. (Optionally) Explain behaviour to humans via exported or streamed replay steps.

This repository implements that core as a **single-threaded, deterministic** foundation, plus opt-in tooling for benchmarks and visualisation.

## Why it is useful as a C++ systems project

- **Real structure:** modular headers, CMake, GoogleTest, multiple CLI modes, benchmark harnesses, optional UI.
- **Correctness first:** **160** automated tests (`./scripts/verify.sh`) cover parsers, book, engine, binary protocol, SPSC pipeline, and visualisation export/stream helpers.
- **Performance awareness:** Release benchmarks, baseline docs, profiler-guided 6E/6G work—**without** claiming production throughput.
- **Clear boundaries:** parsers vs `MatchingEngine` vs `OrderBook`; replay path separate from engine path; UI and viz off the hot path by default.
- **Extension path:** documented in [ROADMAP.md](ROADMAP.md) and prioritised in [FUTURE_IMPROVEMENTS.md](FUTURE_IMPROVEMENTS.md).

## What this project is not

- Not production-ready exchange software.
- Not connected to real brokers or exchanges.
- Not multi-threaded or lock-free in the default matching hot path.
- Not a strategy, PnL, portfolio, auth, or cloud product.
- Not a substitute for Release benchmarks when evaluating performance ([BENCHMARKING.md](BENCHMARKING.md)).

## Current system components

| Component | Role |
|-----------|------|
| `OrderBook` | Resting orders, price levels, FIFO, lookup by order ID |
| `MarketDataParser` | Replay path: CSV → `MarketEvent` |
| `OrderCommandParser` | Engine path: CSV → `OrderCommand` |
| `MatchingEngine` | Matching, market/modify, cancel |
| `BinaryProtocol` / `BinaryCommandReader` | OBK1 encode/decode and `.obk` file I/O |
| `ReplayVisualisationWriter` / `ReplayVisualisationStreamServer` | Opt-in NDJSON export and localhost SSE (7B) |
| `SpscRingBuffer` / `SpscCommandPipeline` | Optional ingest wrapper (6H) |
| `WorkloadGenerator` | Deterministic synthetic workloads |
| Benchmark executables | Engine, binary protocol, ring-buffer pipeline |
| `ui/replay-visualiser/` | Optional React UI: file, live stream, run summary (7A–7C) |

## Completed milestone track (summary)

| Track | IDs | Highlights |
|-------|-----|------------|
| Core | 1–4 | Replay, matching engine, market/modify, benchmarks |
| Binary | 5C–5F | OBK1 decoder, file I/O, `--binary-engine`, benchmark integration |
| Performance | 6A–6H | Baselines, streaming read, `process_into`, profiling, reserve tuning, SPSC |
| Visualisation | 7A–7C | NDJSON export, live SSE, UI live-follow, run summary metrics |

Queue detail: [MILESTONE_QUEUE.md](MILESTONE_QUEUE.md). **CURRENT:** **8B** — CI and correctness hardening (not started). **8A** done: [DEMO_GUIDE.md](DEMO_GUIDE.md).

## Long-term vision (candidate, not queued)

From [ROADMAP.md](ROADMAP.md)—implement only after planning and tests:

- TCP order gateway (commands over the wire)
- Market data publisher (trades / BBO from engine events)
- Persistence and deterministic replay log
- Replication research (after persistence)

**Proposed near-term packaging:** milestones **8A** (docs/demo), **8B** (CI/correctness), **8C** (choose next systems feature)—see [FUTURE_IMPROVEMENTS.md](FUTURE_IMPROVEMENTS.md).

## Portfolio value (skills demonstrated)

- Modern C++20, clear module boundaries, CMake + GoogleTest
- Price-time matching, command/event separation, binary wire format
- Benchmarking and profiling discipline (dated samples, no hype)
- Optional full-stack demo path: C++ export/stream + React visualiser
- Documented backlog and scope limits ([FUTURE_IMPROVEMENTS.md](FUTURE_IMPROVEMENTS.md))

Treat benchmark numbers as **machine-dependent samples** for comparison on your hardware, not guarantees of production performance.
