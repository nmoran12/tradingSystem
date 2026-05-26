# Project Overview

## cpp-low-latency-orderbook

A C++20 educational exchange simulator focused on market data, order-book reconstruction, and deterministic order matching with latency measurement. The project demonstrates how exchange-style systems separate **commands** (client input) from **events** (engine output), maintain price-time priority, and measure per-operation performance—without connecting to live markets or handling real money.

## Problem it solves

Trading systems need a fast, correct core that can:

1. Maintain resting liquidity in price-time priority.
2. Match crossing orders and emit trade events.
3. Accept structured commands (new, cancel, modify) and reject invalid input.
4. Measure how long each operation takes on a given machine.

This repository implements that core as a **single-threaded, deterministic** foundation suitable for learning, testing, and future low-latency extensions.

## Why it is useful as a C++ systems project

- **Real structure, not a toy snippet:** modular headers, CMake, GoogleTest, CLI modes, and a benchmark harness.
- **Correctness first:** 56 automated tests cover parsers, book state, matching, market/modify behaviour, and workload determinism.
- **Performance awareness:** `LatencyTracker` and `matching_engine_benchmark` report throughput and percentiles on synthetic workloads.
- **Clear extension path:** documented milestones for binary protocol, ring buffers, networking, persistence, and replication—without implementing them prematurely.

## What this project is not (yet)

- Not production-ready exchange software.
- Not connected to real brokers or exchanges.
- Not multi-threaded or lock-free in the hot path.
- Not a strategy, PnL, or portfolio system.
- Not a distributed or replicated trading cluster.

## Current system components

| Component | Role |
|-----------|------|
| `OrderBook` | Resting order storage, price levels, FIFO queues, lookup by order ID |
| `MarketEvent` + `MarketDataParser` | Milestone 1 replay path (`ADD` / `CANCEL` / `EXECUTE`) |
| `MatchingEngine` | Trading logic: limit/market match, modify, cancel |
| `OrderCommand` | Client-style input (`NewOrder`, `CancelOrder`, `ModifyOrder`) |
| `EngineEvent` | Engine output (`OrderAccepted`, `Trade`, `BookUpdate`, etc.) |
| `OrderCommandParser` | Command CSV → `OrderCommand` |
| `EngineEventPrinter` | Human-readable CLI event output |
| `WorkloadGenerator` | Deterministic synthetic command streams |
| `matching_engine_benchmark` | Throughput/latency benchmark executable |
| `LatencyTracker` | Nanosecond stats (avg, min, max, p50/p95/p99) |
| GoogleTest + CMake | Build, test, and CI-friendly workflow |

## Completed milestones

### Milestone 1: OrderBook and CSV replay

- CSV parser for `MarketEvent`
- Domain types (`MarketEvent`, `Order`, `Trade`)
- `OrderBook` with price-time priority
- `LatencyTracker`
- GoogleTest coverage
- Replay CLI path (`--replay` or legacy single-arg)

### Milestone 2: MatchingEngine

- `OrderCommand` input model
- `EngineEvent` output model
- Automatic limit-order matching
- Partial and full fills
- Cancels and duplicate-order rejection

### Milestone 3: Complete exchange command layer

- Market orders (no rest; unfilled quantity cancelled)
- Modify orders (cancel-and-reinsert semantics)
- `OrderCommandParser` for command CSV
- Engine CLI mode (`--engine`)
- `EngineEventPrinter`
- Replay path preserved

### Milestone 4: Benchmarking and profiling harness

- `matching_engine_benchmark` executable
- Deterministic `WorkloadGenerator`
- Throughput and latency metrics (including p50/p95/p99)
- Post-run book invariant sanity checks

## Long-term vision

Evolve from a **correct single-threaded simulator** into a **performance-oriented exchange core** with:

- Compact binary command protocol (Milestone 5)
- SPSC ring-buffer command pipeline (Milestone 6)
- TCP order gateway (Milestone 7)
- Market data publisher (Milestone 8)
- Persistence and deterministic replay (Milestone 9)
- Replication/failover research (Milestone 10)

Each step should preserve existing tests and architectural boundaries unless a milestone explicitly changes them.

## Portfolio value (skills demonstrated)

- **Modern C++20:** `enum class`, `std::optional`, RAII, clear module boundaries
- **Data structures:** `std::map` price levels, `std::list` FIFO, `std::unordered_map` order lookup
- **Deterministic matching:** price-time priority, resting-price execution, explicit event ordering
- **Command/event architecture:** input commands vs output events, separate replay path
- **Testing:** unit tests for parsers, book, engine, market/modify edge cases
- **Benchmarking:** fixed-seed workloads, latency percentiles, invariant checks
- **Systems thinking:** thin `main.cpp`, parsers separate from engine, benchmark code isolated
- **Foundation for low-latency work:** documented path toward binary feeds, ring buffers, and gateways—without claiming production latency today

Tone note: treat benchmark numbers as **machine-dependent samples** for relative comparison, not as guarantees of production performance.
