# Active Milestone

**Source of truth** for `/run-active-milestone` and `/review-milestone`.

**Project root:** `cpp-low-latency-orderbook/`  
**Queue position:** See [MILESTONE_QUEUE.md](MILESTONE_QUEUE.md) — **7C is CURRENT**

---

## Milestone 7C — UI Metrics and Benchmark Overlay

| Field | Value |
|-------|--------|
| **Status** | IN PROGRESS — slice 1 (run summary panel) shipped; review before close |
| **Parent** | Milestone 7 — Optional visualisation (offline-first) |
| **Prerequisite** | 7A + 7B complete — file export, live SSE stream, and UI replay visualiser |
| **Test baseline** | 160 tests; UI builds via `cd ui/replay-visualiser && npm run build` |

### Model recommendation

**Cursor Auto** is fine for a first slice that aggregates metrics **client-side** from loaded or streamed replay steps (no C++ changes). Prefer a **stronger model** if the slice adds new C++ export fields, benchmark ingestion, or schema changes — review scope before coding.

---

### Goal

Show **informational** run and replay summary metrics in the existing replay visualiser (command counts, trade totals, resting-book stats, optional timing hints) so users can understand a replay or demo run at a glance — **without** claiming Release benchmark throughput/latency and **without** touching matching or benchmark hot paths.

### What already exists (do not reimplement)

| Deliverable | Location / notes |
|-------------|------------------|
| Replay steps | `schemaVersion: 1` records from file export, scenarios, or live SSE |
| UI shell | `ui/replay-visualiser/` — ladder, tape, BBO, chart, playback, live-follow |
| Release benchmarks | `matching_engine_benchmark`, `binary_protocol_benchmark` — separate workflow ([BENCHMARKING.md](BENCHMARKING.md)) |
| Docs | [REPLAY_VISUALISER.md](REPLAY_VISUALISER.md) |

### Slice plan

| Slice | Deliverable | Status |
|-------|-------------|--------|
| **1** | Client-side `runMetrics.ts` + Run summary panel + docs | **Done** |
| **2** | Optional polish (e.g. “at current step” vs “full run” toggle) | Deferred |

**Slice 1 shipped:** `ui/replay-visualiser/src/runMetrics.ts`, `RunSummaryPanel.tsx`; metrics from all loaded steps; live stream updates as steps append.

**Defer:** benchmark file ingestion; C++ latency export; charts comparing runs; 7B polish (reconnect, demo script).

### Likely files touched

| Area | Examples |
|------|----------|
| UI | `ui/replay-visualiser/src/App.tsx`, optional `src/runMetrics.ts`, `styles.css` |
| Docs | [REPLAY_VISUALISER.md](REPLAY_VISUALISER.md), [MILESTONE_7_PLAN.md](MILESTONE_7_PLAN.md) §7C |

### Constraints

- **No** changes to `MatchingEngine` / `OrderBook` matching logic unless explicitly approved for a tiny, gated export field.
- **No** instrumentation in `matching_engine_benchmark` or `binary_protocol_benchmark` by default.
- **No** performance or throughput claims from UI-derived numbers.
- Keep file export, live stream, and scenario loading behaviour unchanged.
- Do not commit `node_modules/`, `dist/`, or `profiling/`.

### Out of scope

- Replacing Release benchmarks or [PERFORMANCE_BASELINE.md](PERFORMANCE_BASELINE.md) workflows
- Live exchange connectivity or production dashboards
- Full book depth export (7A follow-up)
- TCP order gateway ([ROADMAP.md](ROADMAP.md) Milestone 7 TCP)

### Acceptance criteria (for 7C close)

- [x] `./scripts/verify.sh` passes (160 tests; C++ unchanged)
- [x] `npm run build` passes under `ui/replay-visualiser/`
- [x] Run summary panel for file, scenario, and live-loaded replays
- [x] Docs state informational-only scope ([REPLAY_VISUALISER.md](REPLAY_VISUALISER.md))
- [x] No matching-core or benchmark hot-path changes

Human review before `/advance-milestone`.

### Required verification

```bash
./scripts/verify.sh
cd ui/replay-visualiser && npm run build
```

### Key docs

- [MILESTONE_7_PLAN.md](MILESTONE_7_PLAN.md) §7C
- [REPLAY_VISUALISER.md](REPLAY_VISUALISER.md)
- [BENCHMARKING.md](BENCHMARKING.md)
- [DEVELOPMENT_RULES.md](DEVELOPMENT_RULES.md)

### Human review before advance

After `/run-active-milestone` completes, run `/review-milestone`, then human approval before `/advance-milestone`.
