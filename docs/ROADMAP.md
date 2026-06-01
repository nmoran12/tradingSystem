# Roadmap

Staged delivery plan for `cpp-low-latency-orderbook`. Completed milestones are frozen unless a later milestone explicitly requires a compatible extension.

**Post-7C:** The ordered queue through **7C** is complete. **Planned** improvements (CI, demo packaging, extra tests) and proposed milestones **8A–8C** are in [FUTURE_IMPROVEMENTS.md](FUTURE_IMPROVEMENTS.md). Nothing in that backlog is implemented unless this repo already contains it.

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

### Milestone 7 — Replay visualisation (7A, 7B, 7C) — **DONE**

**Goal:** Optional replay visualiser and export/stream paths without contaminating the performance-critical C++ core.

| Slice | Status | Shipped (summary) |
|-------|--------|-------------------|
| **7A** | Done | `--export-visualisation` NDJSON; React UI (file/scenarios); `schemaVersion: 1` |
| **7B** | Done | `--stream-visualisation` localhost HTTP/SSE; UI `EventSource` live-follow |
| **7C** | Done | Client-side run summary metrics panel (informational; not Release benchmarks) |

**Docs:** [REPLAY_VISUALISER.md](REPLAY_VISUALISER.md), [REPLAY_VISUALISER_ROADMAP.md](REPLAY_VISUALISER_ROADMAP.md), [MILESTONE_7_PLAN.md](MILESTONE_7_PLAN.md).

**Follow-ups (planned, not done):** See [FUTURE_IMPROVEMENTS.md](FUTURE_IMPROVEMENTS.md) — demo script, CI, full-depth export, UI polish.

---

### Milestone 7 (systems): TCP order gateway — **candidate, not started**

**Goal:** Accept remote commands over TCP and return events/acks.

**Key features:**

- Listen/accept, framed messages (binary or length-prefixed)
- Map wire format → `OrderCommand` → `MatchingEngine`
- Stream `EngineEvent` responses
- Engine remains free of socket code (gateway is separate module)

**Acceptance:** Integration tests with local client; engine unit tests unchanged.

**Out of scope:** TLS, auth, rate limits, production hardening.

---

### Milestone 8: Market data publisher — **candidate, not started**

**Goal:** Publish trades and top-of-book (and optional depth) from `EngineEvent` stream.

**Key features:**

- Trade notifications
- BBO updates on book change
- Optional depth snapshots
- Subscriber interface (in-process first)

**Out of scope:** Multicast, external feed formats.

---

### Milestone 9: Persistence and deterministic replay — **candidate, not started**

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

---

## Future improvements and proposed queue (8A–8C)

Detailed backlog (priority, difficulty, resume value, scope limits): **[FUTURE_IMPROVEMENTS.md](FUTURE_IMPROVEMENTS.md)**.

**Proposed** next queue rows (not CURRENT until human approval)—see [MILESTONE_QUEUE.md](MILESTONE_QUEUE.md):

| ID | Name | Purpose |
|----|------|---------|
| **8A** | Documentation and Demo Packaging | Accurate README, architecture-at-a-glance, demo instructions, screenshot/GIF placeholders |
| **8B** | CI and Correctness Hardening | GitHub Actions; binary vs CSV equivalence; invariants; stream test coverage |
| **8C** | Next Systems Extension Decision | Plan and choose: TCP gateway vs persistence/replay log vs market data publisher |

These are **planning milestones**; no CI workflow, demo script, or new systems code exists until implemented.
