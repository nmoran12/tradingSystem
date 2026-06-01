# Architecture

This document describes module boundaries and invariants for `cpp-low-latency-orderbook`. The **detailed** diagrams below were written around Milestones 1–4; the repo has since added **binary OBK1** (5), **benchmarks / SPSC** (6), and **optional visualisation** (7) without a full rewrite of every diagram here.

**For an up-to-date picture:** see the architecture-at-a-glance section in [README.md](../README.md) and paths for `protocol/`, `viz/`, and `ui/replay-visualiser/`. Planned doc updates: [FUTURE_IMPROVEMENTS.md](FUTURE_IMPROVEMENTS.md).

## High-level architecture

The system has **two input paths** that should remain separate unless there is an explicit design reason to merge them:

1. **Replay path (Milestone 1):** historical market events applied directly to `OrderBook`.
2. **Engine path (Milestones 2–4):** client commands processed by `MatchingEngine`, which owns matching logic and updates `OrderBook`.

```
                    ENGINE PATH
Input CSV / future network client
        |
        v
Parser / Command Builder          (OrderCommandParser today)
        |
        v
OrderCommand                      (NewOrder | CancelOrder | ModifyOrder)
        |
        v
MatchingEngine                    (matching, modify, market logic)
        |
        v
OrderBook                         (resting state only)
        |
        v
EngineEvent                       (OrderAccepted, Trade, BookUpdate, ...)
        |
        v
Trades / Book Updates / Logs / Benchmarks / Future Feed
        (EngineEventPrinter, benchmark counters, future publisher)


                    REPLAY PATH (separate)
MarketEvent CSV
        |
        v
MarketDataParser
        |
        v
MarketEvent                       (ADD | CANCEL | EXECUTE)
        |
        v
OrderBook.apply_event()           (no MatchingEngine)
        |
        v
Depth / latency stats
```

## Component responsibilities

### OrderBook (`include/order_book/`)

**Layer:** storage / data structure.

**Owns:**

- Resting buy and sell books (price → FIFO list of orders)
- `order_id` → location lookup for O(1) cancel/reduce
- Best bid/ask, spread, depth printing, per-price quantity

**Provides (among others):**

- `add_order`, `cancel_order`, `execute_order` / `reduce_order`
- `peek_best_bid`, `peek_best_ask`, `contains_order`, `get_order`
- `active_order_count`, `total_resting_quantity`, `validate_invariants`
- `apply_event(MarketEvent)` for replay path

**Must not:**

- Decide crossing/matching policy for new client orders
- Parse files or network messages
- Emit `EngineEvent` or trade lifecycle for engine path (engine does that)

---

### MatchingEngine (`include/matching_engine/`)

**Layer:** trading logic.

**Accepts:** `OrderCommand`  
**Emits:** `std::vector<EngineEvent>`

**Handles:**

- Limit orders: cross, partial fill, rest remainder
- Market orders: consume liquidity; cancel unfilled remainder (never rest)
- Modify orders: cancel-and-reinsert; re-run matching if price crosses
- Cancels and duplicate-ID rejection

**Must not:**

- Parse CSV/binary files
- Perform I/O or networking
- Expose internal book containers (uses `OrderBook` public API only)

---

### MarketEvent replay path (`include/market_data/MarketEvent.hpp`, `MarketDataParser`)

**Layer:** compatibility / historical feed simulation.

**Purpose:** Replay exchange-style **events** (`ADD`, `CANCEL`, `EXECUTE`) into `OrderBook` without going through `MatchingEngine`.

**Must remain separate** from engine command mode so Milestone 1 tests and workflows keep working.

---

### OrderCommand (`include/matching_engine/OrderCommand.hpp`)

**Layer:** client input model (future external API shape).

**Fields:** `type`, `order_id`, `side`, `order_type` (Limit/Market), `price`, `quantity`, optional `symbol`.

**Command types:** `NewOrder`, `CancelOrder`, `ModifyOrder`.

---

### EngineEvent (`include/matching_engine/EngineEvent.hpp`)

**Layer:** deterministic engine output.

**Types:** `OrderAccepted`, `OrderRejected`, `OrderCancelled`, `Trade`, `BookUpdate`.

**`MatchTrade`:** aggressive vs resting order id, price, quantity (distinct from `order_book::Trade` used in Milestone 1 domain model).

**Future uses:** logging, replay, market data fan-out, network responses.

---

### OrderCommandParser (`include/market_data/OrderCommandParser.hpp`)

**Layer:** parsing only.

**CSV columns:** `type,order_id,side,order_type,price,quantity`  
**Commands:** `NEW`, `CANCEL`, `MODIFY`  
**Order types:** `LIMIT`, `MARKET`

**Must not:** contain matching logic or mutate `OrderBook`.

---

### EngineEventPrinter (`include/matching_engine/EngineEventPrinter.hpp`)

**Layer:** presentation.

**Purpose:** Format `EngineEvent` for CLI/logging.

**Must not:** contain trading logic.

---

### LatencyTracker (`include/metrics/`)

**Layer:** measurement only.

**Purpose:** Record durations (nanoseconds), compute count/min/max/avg and percentiles (nearest-rank p50/p95/p99).

**Used by:** `main.cpp` (replay and engine modes), `matching_engine_benchmark`.

---

### Benchmarks (`include/benchmarks/`, `benchmarks/`)

**WorkloadGenerator:** deterministic synthetic `OrderCommand` sequences (fixed seed, configurable mix).

**matching_engine_benchmark:** drives `MatchingEngine`, records per-command latency, prints throughput and runs `OrderBook::validate_invariants()`.

**Must not:** alter production matching behaviour; keep in separate target from core library consumers.

---

### main.cpp (`src/main.cpp`)

**Layer:** thin orchestration.

**Modes:**

- `--replay <market_events.csv>` (and legacy single-arg replay)
- `--engine <order_commands.csv>`

**Should stay thin:** parse → loop → print → metrics.

## Current module layout

```
include/
  market_data/      MarketEvent, MarketDataParser, OrderCommandParser
  order_book/       Order, Trade, OrderBook
  matching_engine/  OrderCommand, EngineEvent, MatchingEngine, EngineEventPrinter
  metrics/          LatencyTracker
  benchmarks/       WorkloadGenerator
src/                implementations + main.cpp
benchmarks/         matching_engine_benchmark.cpp
tests/              GoogleTest (160 tests via ./scripts/verify.sh)
data/               sample_events.csv, sample_commands.csv
```

## Future module boundaries (not implemented)

| Module | Status | Notes |
|--------|--------|--------|
| `protocol/` binary OBK1 | **Shipped** (M5) | Maps bytes ↔ `OrderCommand`; see [BINARY_PROTOCOL.md](BINARY_PROTOCOL.md) |
| `concurrency/` SPSC ring buffer | **Shipped** (M6) | Pipeline around engine; engine stays single-threaded |
| `viz/` export + stream | **Shipped** (M7) | NDJSON export and localhost SSE; see [REPLAY_VISUALISER.md](REPLAY_VISUALISER.md) |
| TCP gateway | **Candidate** | Submits commands; no logic in engine — see [ROADMAP.md](ROADMAP.md) |
| Market data publisher | 8 | Consumes `EngineEvent` stream |
| Persistence / replay log | 9 | Append-only command/event log |
| Replication | 10 | After persistence is solid |

## Important invariants

**Book state**

- At most one active entry per `order_id` in `order_lookup_`.
- FIFO within a price level (list order + iterator stability on reduce).
- Buy book: highest price first; sell book: lowest price first.
- `validate_invariants()`: no crossed book (best bid < best ask when both exist), positive quantities, lookup count matches orders in books.

**Matching**

- Trades execute at **resting** order price.
- Market orders never rest; unfilled quantity → `OrderCancelled`.
- Modify uses **cancel-and-reinsert** (FIFO position not preserved).

**Architecture boundaries (do not violate)**

| Rule | Violation example |
|------|-------------------|
| Parsers only parse | Matching inside `OrderCommandParser` |
| Engine owns match logic | `OrderBook` auto-matching on `add_order` for engine path |
| Book owns storage | `MatchingEngine` storing parallel order maps |
| Printer only formats | Trade generation in `EngineEventPrinter` |
| Benchmarks don't change core | `#ifdef BENCHMARK` altering match results |
| Keep replay path working | Removing `apply_event` or breaking CSV replay tests |

## Data flow summary

| Path | Input | Processor | Output |
|------|--------|-----------|--------|
| Replay | `MarketEvent` CSV | `OrderBook` | Updated book, depth |
| Engine | `OrderCommand` CSV | `MatchingEngine` → `OrderBook` | `EngineEvent` list, updated book |
| Benchmark | `WorkloadGenerator` | `MatchingEngine` → `OrderBook` | Metrics + invariant check |

Merging replay into engine (or vice versa) should only happen via an **explicit adapter** (e.g. converting `MarketEvent` to `OrderCommand`), not by entangling parsers and engines.
