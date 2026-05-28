# Roadmap

Staged delivery plan for `cpp-low-latency-orderbook`. Completed milestones are frozen unless a later milestone explicitly requires a compatible extension.

---

## Completed milestones

### Milestone 1: OrderBook and CSV replay — **DONE**

**Goal:** Reconstruct a live order book from historical market events and measure replay latency.

**Key features:**

- `MarketEvent` domain types (`ADD`, `CANCEL`, `EXECUTE`)
- `MarketDataParser` (CSV, header skip, validation)
- `OrderBook` with price-time priority
- `LatencyTracker`
- Replay CLI

**Acceptance:** 20 tests (parser, book, latency) + replay CLI runs on `data/sample_events.csv`.

**Status:** Complete. Do not break `OrderBook::apply_event` or `MarketDataParser` tests.

---

### Milestone 2: MatchingEngine — **DONE**

**Goal:** Automatic limit-order matching with deterministic trade events.

**Key features:**

- `OrderCommand` / `EngineEvent` separation
- Limit match, partial/full fill, rest remainder
- Cancel by order ID, duplicate rejection
- Price-time priority preserved

**Acceptance:** 12 `MatchingEngineTest` cases + all Milestone 1 tests still pass.

**Status:** Complete (32 tests total after M2).

---

### Milestone 3: Complete exchange command layer — **DONE**

**Goal:** Exchange-style simulator with market/modify commands and engine CLI.

**Key features:**

- Market orders (no rest; cancel remainder)
- Modify orders (cancel-and-reinsert)
- `OrderCommandParser` (command CSV)
- `--engine` CLI + `EngineEventPrinter`
- Book introspection APIs

**Acceptance:** Parser, market, and modify tests; replay path unchanged.

**Status:** Complete (56 tests total after M3).

---

### Milestone 4: Benchmarking and profiling harness — **DONE**

**Goal:** Performance measurement on deterministic synthetic workloads.

**Key features:**

- `matching_engine_benchmark`
- `WorkloadGenerator` (seeded mix: 70% limit, 10% market, 10% cancel, 10% modify)
- Throughput + latency percentiles
- Post-run `validate_invariants()`

**Acceptance:** Benchmark runs; workload determinism tests pass; no change to matching semantics.

**Status:** Complete. Sample results documented in [BENCHMARKING.md](BENCHMARKING.md).

---

## Future milestones

### Milestone 5: Binary protocol and feed handler

**Goal:** Compact binary encoding of `OrderCommand` for lower-overhead file/network input.

**Key features:**

- Fixed-width (or simple TLV) message format with version field
- Encoder/decoder ↔ `OrderCommand`
- Binary file reader + optional `--binary-engine` CLI
- Validation of enums, sizes, endianness

**Acceptance criteria:**

- Round-trip binary ↔ `OrderCommand`
- Malformed messages rejected with clear errors
- CSV engine mode and replay mode unchanged
- All 56 existing tests pass; new protocol tests pass

**Out of scope:** FIX/ITCH, networking, compression, encryption.

**Plan:** [MILESTONE_5_PLAN.md](MILESTONE_5_PLAN.md)

---

### Milestone 6: Performance baseline and optimisation groundwork

**Goal:** Establish repeatable Release-mode performance baselines before making targeted optimisation changes.

**Key features:**

- Release benchmark script and baseline document
- Binary protocol and matching engine benchmark comparison workflow
- Future streaming binary replay path and allocation reduction pass
- Optional SPSC queue design/implementation after profiling justifies it

**Acceptance criteria:**

- `./scripts/benchmark_release.sh` builds and runs benchmark targets
- Local baseline methodology and results documented
- All existing tests pass unchanged
- No matching, order book, protocol, or CLI behaviour changes

**Out of scope:** SPSC implementation in 6A, MPMC, lock-free engine internals, TCP.

**Plan:** [MILESTONE_6_PLAN.md](MILESTONE_6_PLAN.md)

---

### Milestone 7A: Replay Visualiser UI

**Goal:** Build an optional replay visualiser that helps users understand and debug order book behaviour without contaminating the performance-critical C++ core.

**Recommended architecture:**

- The C++ engine remains headless and performance-focused.
- Replay paths export deterministic, visualisation-friendly output (JSON or NDJSON).
- A separate frontend app reads those outputs (implementation detail intentionally flexible; e.g. React + Vite).
- UI is optional and kept out of benchmark and Release measurement workflows.

**Scope (first prototype):**

- Export visualisation-friendly replay data:
  - order book snapshots (or top-of-book / limited depth)
  - trades
  - best bid / best ask
  - spread
  - command sequence number or timestamp
  - basic replay metrics (counts, totals)
- UI prototype views:
  - order book ladder
  - trade tape
  - best bid/ask + spread display
  - replay controls (step, play, pause, reset)
  - load replay output from file initially
- Keep UI separate from core:
  - no UI code inside `MatchingEngine` or `OrderBook`
  - no frontend dependency in benchmark or Release paths
  - no added instrumentation in hot paths unless explicitly gated

**Acceptance criteria:**

- Clear docs for generating replay visualisation output and running the UI
- Visualiser reads a recorded replay output file and renders the basic views
- Core correctness tests and benchmarks remain unchanged

**Out of scope:**

- React/Vite implementation details until the milestone starts
- WebSocket/HTTP servers
- GUI frameworks in C++ (Qt, Dear ImGui, etc.)
- Live trading simulation or real exchange connectivity
- Changes to matching logic or replay semantics

**Plan:** [MILESTONE_7_PLAN.md](MILESTONE_7_PLAN.md)

---

### Milestone 7: TCP order gateway

**Goal:** Accept remote commands over TCP and return events/acks.

**Key features:**

- Listen/accept, framed messages (binary or length-prefixed)
- Map wire format → `OrderCommand` → `MatchingEngine`
- Stream `EngineEvent` responses
- Engine remains free of socket code (gateway is separate module)

**Acceptance:** Integration tests with local client; engine unit tests unchanged.

**Out of scope:** TLS, auth, rate limits, production hardening.

---

### Milestone 8: Market data publisher

**Goal:** Publish trades and top-of-book (and optional depth) from `EngineEvent` stream.

**Key features:**

- Trade notifications
- BBO updates on book change
- Optional depth snapshots
- Subscriber interface (in-process first)

**Out of scope:** Multicast, external feed formats.

---

### Milestone 9: Persistence and deterministic replay

**Goal:** Durable command/event log and recovery.

**Key features:**

- Append-only command log
- Periodic snapshots
- Crash recovery simulation
- Replay produces identical book/event sequence

**Out of scope:** Full production WAL tuning.

---

### Milestone 10: Replication / failover

**Goal:** Leader/follower research on replicated command log.

**Key features:**

- Replicated log apply on followers
- Node restart simulation
- Consistency tests

**Out of scope:** Production Raft cluster; implement only after Milestone 9 is solid.

**Note:** Raft-inspired design only when persistence and deterministic replay are trustworthy.

---

## Sequencing rationale

| Order | Why |
|-------|-----|
| 5 before 6 | Binary messages are natural ring-buffer payload |
| 6 before 7 | Pipeline pattern before real I/O |
| 7 before 8 | Commands before outbound market data |
| 9 before 10 | Durability before distributed consensus |

Do not skip ahead without updating docs and tests for the current milestone.
