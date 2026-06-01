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
| 8 | 6D | Benchmark Stability and Profiling Report | Done |
| 9 | 6E | Targeted Hot-Path Optimisation | Done |
| 10 | 6F | Memory Pool / Object Pool | Done |
| 11 | 6G | Order Book Data-Structure Optimisation | Done |
| 12 | 6H | Optional SPSC Queue | Done |
| 13 | 7A | Replay Visualiser UI | Done |
| 14 | 7B | Live Replay Streaming Interface | Done |
| 15 | 7C | UI Metrics and Benchmark Overlay | Done |
| 16 | 8A | Documentation and Demo Packaging | Done |
| 17 | **8B** | **CI and Correctness Hardening** | **CURRENT** |

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

### 6D — Benchmark Stability and Profiling Report

Repeated benchmark script, profiling methodology doc, and guidance before further optimisation.

**Plan reference:** [PROFILING_REPORT.md](PROFILING_REPORT.md), [BENCHMARKING.md](BENCHMARKING.md)

---

### 6E — Targeted Hot-Path Optimisation

Profiler-guided changes in confirmed hot paths only; behaviour unchanged. Engine-only benchmark profiling mode; single cancel-path lookup removal; baseline and profiling docs updated (no claimed throughput win).

---

### 6F — Memory Pool / Object Pool

Scoped event-buffer reuse via `MatchingEngine::process_into` (caller-owned `std::vector<EngineEvent>`); `process()` kept as a compatibility wrapper. Hot benchmark/CLI loops reuse one scratch vector. Engine-owned move-return buffer was evaluated and rejected (ownership transfers to caller). Profiler-justified, 127 tests passing, local benchmark deltas documented as machine-dependent.

---

### 6G — Order Book Data-Structure Optimisation

**Completed.** Narrow, profiler-backed book lookup tuning only — no container family swap.

| Outcome | Detail |
|---------|--------|
| **Slice 1** | `OrderBook::reserve_active_orders`, `MatchingEngine::reserve_book_capacity`, benchmark `reserve_book_capacity(command_count / 10)`; tests for parity |
| **Profiling** | Engine-only `sample`: `order_lookup_` **rehash pressure much reduced** after reserve |
| **Throughput** | Repeated Release benchmarks **noisy** — **no clear throughput win** claimed |
| **Slice 2** | Measurement-only allocation attribution (`sample`, `malloc_history`, failed `xctrace` attach): **mixed** list + hash + some map signals on resting adds |
| **Not done** | `std::list` / `std::map` redesign — **not justified** by evidence |

**Docs:** [PROFILING_REPORT.md](PROFILING_REPORT.md) §6G, [PERFORMANCE_BASELINE.md](PERFORMANCE_BASELINE.md) §6G, [PERFORMANCE_ROADMAP.md](PERFORMANCE_ROADMAP.md).

---

### 6H — Optional SPSC Queue

**Completed.** Correctness-first SPSC ingest wrapper around `MatchingEngine`; no matching or book internals changed.

| Deliverable | Detail |
|-------------|--------|
| **`SpscRingBuffer<T>`** | Header-only fixed-capacity ring; acquire/release atomics; deterministic tests |
| **`SpscCommandPipeline`** | Single-threaded enqueue/drain → `process_into` |
| **Equivalence tests** | Hand-written sequences + `WorkloadGenerator` (100 / 1 000 commands, seed 42); large-queue and interleaved small-queue paths |
| **`ring_buffer_pipeline_benchmark`** | Side-by-side direct vs pipeline on same workload |
| **Benchmark (one local run)** | Pipeline **slower** (~6.3M vs ~8.4M cmd/s at 100k/seed 42) — expected enqueue/drain overhead; **no throughput improvement claim** |
| **Not done** | Multithreaded pipeline, output ring, pipeline optimisation |

**Docs:** [BENCHMARKING.md](BENCHMARKING.md) (`ring_buffer_pipeline_benchmark`), [PERFORMANCE_BASELINE.md](PERFORMANCE_BASELINE.md) §6H.

**Plan reference:** [MILESTONE_6_PLAN.md](MILESTONE_6_PLAN.md) (as scoped)

---

### 7A — Replay Visualiser UI

**Completed.** Offline replay visualisation without coupling the UI or export path to benchmark hot loops.

| Outcome | Detail |
|---------|--------|
| **NDJSON export** | Opt-in `--binary-engine <file.obk> --export-visualisation <replay.ndjson>`; `schemaVersion: 1`; shallow BBO depth per side (`src/main.cpp`) |
| **UI** | React + Vite spike at `ui/replay-visualiser/` (ladder, tape, BBO, playback, chart) |
| **Docs** | [REPLAY_VISUALISER.md](REPLAY_VISUALISER.md) — exact flag name; bundled `public/*.ndjson` labelled as **demo fixtures** where multi-level depth exceeds exporter |
| **Tests** | `tests/test_cli_visualisation_export.cpp` — CLI success, one line per command, field shape, trade line on cross |
| **Not done (future)** | Full depth export; CSV `--engine` / `--replay` export; `--export-viz` alias; UI speed controls; frontend tests; regenerating rich demo fixtures from CLI |

**Docs:** [REPLAY_VISUALISER.md](REPLAY_VISUALISER.md), [MILESTONE_7_PLAN.md](MILESTONE_7_PLAN.md) §7A.

**Plan reference:** [MILESTONE_7_PLAN.md](MILESTONE_7_PLAN.md)

---

### 7B — Live Replay Streaming Interface

**Completed.** Localhost live replay stream for the visualiser; no matching-core or benchmark hot-path changes.

| Outcome | Detail |
|---------|--------|
| **CLI** | Opt-in `--binary-engine <file.obk> --stream-visualisation <host:port>` (localhost only) |
| **Transport** | HTTP/SSE via `viz::ReplayVisualisationStreamServer`; `EventSource`-friendly |
| **Records** | Reused `schemaVersion: 1` replay visualisation JSON (`viz::ReplayVisualisationWriter`) — same shape as `--export-visualisation` NDJSON lines |
| **Tests** | `tests/test_replay_visualisation_writer.cpp`, `tests/test_replay_visualisation_stream.cpp` (writer, SSE framing, loopback guard, in-process stream client; no flaky CLI subprocess test) |
| **UI** | `ui/replay-visualiser/` live-follow: stream URL, Connect/Disconnect, status, follow-live, malformed-event counter; file upload and bundled scenarios preserved |
| **Docs** | [REPLAY_VISUALISER.md](REPLAY_VISUALISER.md), [REPLAY_VISUALISER_ROADMAP.md](REPLAY_VISUALISER_ROADMAP.md) |
| **Not done (optional polish)** | Reconnect/backpressure; clear-steps control; `scripts/demo-live-replay.sh`; production streaming; live market data |

**Commits:** backend `5758303`; UI `dd1bb22`.

**Docs:** [REPLAY_VISUALISER.md](REPLAY_VISUALISER.md), [MILESTONE_7_PLAN.md](MILESTONE_7_PLAN.md) §7B.

---

### 7C — UI Metrics and Benchmark Overlay

**Completed.** Client-side run summary metrics in the replay visualiser; no C++ schema or benchmark hot-path changes.

| Outcome | Detail |
|---------|--------|
| **Metrics** | `ui/replay-visualiser/src/runMetrics.ts` — totals, trade stats, final BBO/spread, min/max spread, peak resting orders/qty |
| **UI** | `RunSummaryPanel.tsx` — informational disclaimer; works for scenarios, file NDJSON, and live SSE |
| **Docs** | [REPLAY_VISUALISER.md](REPLAY_VISUALISER.md) §Run summary; references [BENCHMARKING.md](BENCHMARKING.md) for real benchmarks |
| **Not done (optional)** | “At current step” vs full-run toggle; benchmark file ingestion; C++ latency on stream path |

**Commit:** `7423903` — Add replay visualiser run summary metrics.

**Docs:** [MILESTONE_7_PLAN.md](MILESTONE_7_PLAN.md) §7C, [REPLAY_VISUALISER_ROADMAP.md](REPLAY_VISUALISER_ROADMAP.md).

---

## Queue status

**CURRENT milestone:** **8B — CI and Correctness Hardening** ([ACTIVE_MILESTONE.md](ACTIVE_MILESTONE.md)).

**8A** is **Done** (commit `f4116b3`). **8C** remains **Proposed** below. Do not implement **8B** until `/run-active-milestone` in a new session.

---

### 8A — Documentation and Demo Packaging

**Completed.** Docs-only milestone; no C++/UI/CI changes.

| Outcome | Detail |
|---------|--------|
| **README** | “What I built”, architecture-at-a-glance, 160-test baseline, honest benchmark links, visualiser summary, no CI implied |
| **Demo guide** | [DEMO_GUIDE.md](DEMO_GUIDE.md) — clone→demo, offline scenarios, NDJSON export, live SSE, fixture honesty, screenshot checklist |
| **Hygiene** | `profiling/` untracked; systems features (TCP, publisher, persistence) labelled planned only |
| **Not done** | `scripts/demo-live-replay.sh`, committed screenshots/GIFs (checklist only) |

**Commit:** `f4116b3` — Polish documentation and demo guide for 8A.

---

### 8B — CI and Correctness Hardening

**Status:** **CURRENT** — not started ([ACTIVE_MILESTONE.md](ACTIVE_MILESTONE.md)).

**Goal:** Project credibility via automated build/test and stronger replay/protocol correctness checks.

**Planned scope (not done yet):**

1. GitHub Actions — `cmake` build, `ctest`, optional `ui/replay-visualiser` `npm run build`.
2. Binary vs CSV equivalence test — same workload, equivalent observable book/trade outcomes.
3. Long-workload invariant tests — `validate_invariants()` after large generated workloads.
4. Stable stream integration coverage — reliable test for `--stream-visualisation` (no flaky subprocess pattern).

**Out of scope for 8B:** MatchingEngine / OrderBook semantic changes; perf regression gates without explicit design; implementing 8C systems features.

**Related backlog:** [FUTURE_IMPROVEMENTS.md](FUTURE_IMPROVEMENTS.md).

---

## Later milestones (Proposed, not CURRENT)

| Order | ID | Name | Status | Goal |
|-------|-----|------|--------|------|
| 18 | **8C** | Next Systems Extension Decision | **Proposed** | Planning only: compare and scope **TCP order gateway** vs **persistence/replay log** vs **market data publisher** ([ROADMAP.md](ROADMAP.md)) before implementation |

**After 8B:** advance to **8C**, then queue one systems milestone from [ROADMAP.md](ROADMAP.md).

**Backlog items** not tied to a single milestone (playback speed, full-depth export, fuzz tests, etc.) live in [FUTURE_IMPROVEMENTS.md](FUTURE_IMPROVEMENTS.md).

---

## Advancing the queue

When a milestone is **done** and reviewed:

1. Human confirms acceptance criteria met.
2. Run **`/advance-milestone`** (docs only).
3. Human reviews updated `ACTIVE_MILESTONE.md` and `MILESTONE_QUEUE.md`.
4. Run **`/run-active-milestone`** in a **new** session when ready to implement the next item.

Completed milestones (1–4, 5A, 5B, 5C, 5D, 5E, 5F, 6A–6H, 7A, 7B, 7C, 8A) are documented in [ROADMAP.md](ROADMAP.md).
