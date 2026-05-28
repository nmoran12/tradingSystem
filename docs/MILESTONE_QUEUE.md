# Milestone Queue

Ordered backlog for **one milestone at a time** automation. Do not implement multiple queue items in a single Cursor session.

## Rule: one milestone at a time

1. Only the milestone marked **CURRENT** in this file may be copied into [ACTIVE_MILESTONE.md](ACTIVE_MILESTONE.md) for implementation.
2. Finish implementation, `./scripts/verify.sh`, and `/review-milestone` before advancing.
3. Use **`/advance-milestone`** only after **human approval** — it updates docs only, no code.
4. Never chain `/run-active-milestone` automatically after `/advance-milestone`.

---

## Queue

| Order | ID | Name | Status |
|-------|-----|------|--------|
| 1 | 5C | Binary Decoder | Done |
| 2 | 5D | Binary File Writer/Reader | Done |
| 3 | 5E | Binary Replay CLI | Done |
| 4 | 5F | Binary Benchmark Integration | Done |
| 5 | 6A | Performance Baseline and Release Benchmark Workflow | Done |
| 6 | 6B | Streaming Binary Replay Path | Done |
| 7 | 6C | Allocation and Copy Reduction Pass | Done |
| 8 | **6D** | Benchmark Stability and Profiling Report | **CURRENT** |
| 9 | 6E | Targeted Hot-Path Optimisation | Queued |
| 10 | 6F | Memory Pool / Object Pool | Queued |
| 11 | 6G | Order Book Data-Structure Optimisation | Queued |
| 12 | 6H | Optional SPSC Queue | Queued |
| 13 | 7A | Replay Visualiser UI | Queued |
| 14 | 7B | Live Replay Streaming Interface | Queued |
| 15 | 7C | UI Metrics and Benchmark Overlay | Queued |

---

## Milestone summaries

### 5C — Binary Decoder

Decode one fixed 64-byte OBK1 v1 message into `DecodedOrderCommand`. See [ACTIVE_MILESTONE.md](ACTIVE_MILESTONE.md).

**Plan reference:** [MILESTONE_5_PLAN.md](MILESTONE_5_PLAN.md) §5C, [BINARY_PROTOCOL.md](BINARY_PROTOCOL.md)

---

### 5D — Binary File Writer/Reader

Read (and optionally write) concatenated 64-byte command files; golden binary fixtures.

**Plan reference:** [MILESTONE_5_PLAN.md](MILESTONE_5_PLAN.md) §5D

---

### 5E — Binary Replay CLI

`--binary-engine` (or equivalent) feeding `MatchingEngine` from binary command file.

**Plan reference:** [MILESTONE_5_PLAN.md](MILESTONE_5_PLAN.md) §5E

---

### 5F — Binary Benchmark Integration

Optional binary ingest path or metrics in benchmark harness (documented, scoped).

**Plan reference:** Extend [BENCHMARKING.md](BENCHMARKING.md); align with Milestone 5 goals

---

### 6A — Performance Baseline and Release Benchmark Workflow

Create a repeatable Release-mode benchmark workflow and document local baseline methodology before optimisation.

**Plan reference:** [BENCHMARKING.md](BENCHMARKING.md), [PERFORMANCE_BASELINE.md](PERFORMANCE_BASELINE.md)

---

### 6B — Streaming Binary Replay Path

Streaming binary reader and benchmark comparison of buffered vs streaming replay.

---

### 6C — Allocation and Copy Reduction Pass

Low-risk allocation/copy reductions in hot and benchmarked paths; baseline before/after documented.

---

### 6D — Benchmark Stability and Profiling Report (CURRENT)

Repeated benchmark script, profiling methodology doc, and guidance before further optimisation.

**Plan reference:** [PROFILING_REPORT.md](PROFILING_REPORT.md), [BENCHMARKING.md](BENCHMARKING.md)

---

### 6E — Targeted Hot-Path Optimisation

Profiler-guided changes in confirmed hot paths only; behaviour unchanged.

---

### 6F — Memory Pool / Object Pool

Optional object pooling for identified hot allocations (scoped, documented).

---

### 6G — Order Book Data-Structure Optimisation

Order book storage improvements after profiling and baseline comparison.

---

### 6H — Optional SPSC Queue

Design and optional implementation of single-producer / single-consumer ingest pipeline.

**Plan reference:** [MILESTONE_6_PLAN.md](MILESTONE_6_PLAN.md) (as scoped)

---

### 7A — Replay Visualiser UI

Optional web-based UI that visualises replay output without adding dependencies or latency impact to the C++ core.

**Plan reference:** [MILESTONE_7_PLAN.md](MILESTONE_7_PLAN.md)

---

### 7B — Live Replay Streaming Interface

Optional follow-up: stream replay output incrementally (still offline / non-production) for a smoother UI experience.

---

### 7C — UI Metrics and Benchmark Overlay

Optional follow-up: display per-run timing and summary metrics in the visualiser for education and debugging (not a substitute for Release benchmarks).

---

## Advancing the queue

When a milestone is **done** and reviewed:

1. Human confirms acceptance criteria met.
2. Run **`/advance-milestone`** (docs only).
3. Human reviews updated `ACTIVE_MILESTONE.md` and `MILESTONE_QUEUE.md`.
4. Run **`/run-active-milestone`** in a **new** session when ready to implement the next item.

Completed milestones (1–4, 5A, 5B, 5C, 5D, 5E, 5F, 6A, 6B, 6C) are documented in [ROADMAP.md](ROADMAP.md).
