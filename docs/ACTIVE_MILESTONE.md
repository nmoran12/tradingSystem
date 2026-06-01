# Active Milestone

**Source of truth** for `/run-active-milestone` and `/review-milestone`.

**Project root:** `cpp-low-latency-orderbook/`  
**Queue position:** See [MILESTONE_QUEUE.md](MILESTONE_QUEUE.md) — **7B is CURRENT**

---

## Milestone 7B — Live Replay Streaming Interface

| Field | Value |
|-------|--------|
| **Status** | READY FOR REVIEW — 7B backend SSE + UI live-follow complete |
| **Parent** | Milestone 7 — Optional visualisation (offline-first) |
| **Prerequisite** | 7A complete — file-based NDJSON export + UI spike |
| **Test baseline** | 160 tests (7B writer + stream server tests) |

### Model recommendation

Prefer a **stronger model** for the first slice if it includes localhost streaming, backpressure, and UI consumption state. **Cursor Auto** is fine for docs-only design or a minimal proof-of-concept with explicit scope; avoid ad-hoc protocol changes without review.

---

### Goal

Let the replay visualiser **follow a binary-engine replay as it runs** by streaming the same NDJSON step records 7A already defines — without changing matching semantics, replay semantics, or Release benchmark hot paths.

The C++ core stays headless. Streaming is **opt-in**, **non-production**, and **localhost-only** for the first implementation.

### What already exists (do not reimplement)

| Deliverable | Location / notes |
|-------------|------------------|
| NDJSON line format | `schemaVersion: 1` — `viz::ReplayVisualisationWriter` |
| File export CLI | `--binary-engine` + `--export-visualisation` |
| Live stream CLI | `--binary-engine` + `--stream-visualisation <host:port>` (localhost SSE) |
| UI (file + live) | `ui/replay-visualiser/` — file/scenarios + `EventSource` live-follow |
| Docs | [REPLAY_VISUALISER.md](REPLAY_VISUALISER.md) |

### Slice plan

| Slice | Deliverable | Status |
|-------|-------------|--------|
| **0** | Roadmap docs: 7A vs 7B, same schema, backend-first, UI deferred | **Done** |
| **1** | Localhost SSE CLI + shared record writer + tests | **Done** |
| **2** | UI live-follow (`EventSource`) | **Done** |
| **3+** | Polish: demo script, reconnect, close 7B review | Optional |

**Defer:** WebSocket transport; production gateway patterns (**Milestone 7 TCP** in [ROADMAP.md](ROADMAP.md) is separate).

See [REPLAY_VISUALISER_ROADMAP.md](REPLAY_VISUALISER_ROADMAP.md).

### Likely files touched

| Area | Examples |
|------|----------|
| CLI / orchestration | `src/main.cpp` or thin `src/viz/` stream helper (gated) |
| Tests | New CLI/stream integration test; reuse binary writer fixtures |
| Docs | [REPLAY_VISUALISER.md](REPLAY_VISUALISER.md), [MILESTONE_7_PLAN.md](MILESTONE_7_PLAN.md) §7B |
| UI (later) | `ui/replay-visualiser/` consumer for stream endpoint |

### Constraints

- Reuse **7A NDJSON shape**; do not change `MatchingEngine` / `OrderBook` matching logic.
- **No** instrumentation in `matching_engine_benchmark` or `binary_protocol_benchmark` by default.
- **No** TLS, auth, or wide-area networking in 7B.
- **No** changes to binary protocol semantics or default CLI behaviour without explicit flags.
- Keep `orderbook_core` free of frontend dependencies.

### Out of scope

- Benchmark overlay UI (**7C**)
- Full book depth export, CSV/`--replay` file export, `--export-viz` alias (7A follow-ups)
- TCP order gateway ([ROADMAP.md](ROADMAP.md) Milestone 7 TCP)
- Claiming throughput/latency improvements from streaming

### Acceptance criteria (7B — met)

- [x] `./scripts/verify.sh` passes (C++ stream tests unchanged)
- [x] Opt-in stream CLI; default binary engine unchanged
- [x] UI connects via `EventSource`, appends `schemaVersion: 1` steps, follows live by default
- [x] File upload and bundled scenarios preserved
- [x] Localhost-only documented; proof-of-concept scope clear

Human review still required before `/advance-milestone`.

### Required verification

```bash
./scripts/verify.sh
```

### Key docs

- [MILESTONE_7_PLAN.md](MILESTONE_7_PLAN.md) §7B
- [REPLAY_VISUALISER.md](REPLAY_VISUALISER.md)
- [ARCHITECTURE.md](ARCHITECTURE.md)
- [DEVELOPMENT_RULES.md](DEVELOPMENT_RULES.md)

### Human review before advance

After `/run-active-milestone` completes, run `/review-milestone`, then human approval before `/advance-milestone`.
