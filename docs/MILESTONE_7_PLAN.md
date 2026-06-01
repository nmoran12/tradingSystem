# Milestone 7 Plan: Replay Visualiser UI

**Status:** 7A **Done**; 7B **Done**; 7C **Done** — visualisation queue complete (see [MILESTONE_QUEUE.md](MILESTONE_QUEUE.md)).  
**Prerequisite:** Replay and engine paths complete; no performance work is blocked on this milestone.

### Implementation snapshot (repo)

| Item | Status |
|------|--------|
| 7A.1 NDJSON export | **Done** — `--binary-engine` + `--export-visualisation`; shallow BBO depth only (`src/main.cpp`) |
| 7A.2 UI prototype | **Done** — `ui/replay-visualiser/` (React + Vite) |
| 7A.3 Documentation | **Done** — [REPLAY_VISUALISER.md](REPLAY_VISUALISER.md) |
| CLI export tests | **Done** — `tests/test_cli_visualisation_export.cpp` |
| 7A follow-ups (optional) | CSV/`--replay` export; full depth; `--export-viz` alias; UI tests; CLI-aligned fixtures |
| 7B streaming | **Done** — localhost SSE + UI live-follow; see [REPLAY_VISUALISER_ROADMAP.md](REPLAY_VISUALISER_ROADMAP.md) |
| 7C metrics overlay | **Done** — `runMetrics.ts` + Run summary panel (`7423903`) |

## Goal

Add an optional UI replay visualiser that helps users understand and debug order book behaviour **without** adding dependencies, instrumentation overhead, or behaviour changes to the performance-critical C++ core.

The UI is for education, debugging, and demo purposes. It is not part of the low-latency benchmark path.

## Architecture principles

- **Core remains headless:** `OrderBook` and `MatchingEngine` do not import or depend on any UI/frontend code.
- **Deterministic exports:** The C++ side exports replay outputs deterministically for a given input and seed.
- **Offline-first (7A), then live stream (7B):** File-based NDJSON export shipped first; localhost SSE streaming reuses the same record shape.
- **Benchmarks stay clean:** No UI output generation in `binary_protocol_benchmark` or `matching_engine_benchmark`.
- **Explicit gating:** If additional export code is needed later, it must be behind explicit modes or build flags so hot paths are unaffected by default.

## Scope

### 7A.1 Export visualisation-friendly replay data (offline)

Design a minimal event/snapshot format that can be produced during a replay run without changing semantics.

Suggested data to export:

- **Sequence identity**
  - replay step index (monotonic)
  - optional timestamp (from decoded commands/events where available)
- **Top-of-book**
  - best bid / best ask
  - spread
- **Depth snapshot (optional for first pass)**
  - top N price levels on each side
  - aggregated quantity at each level
- **Trades**
  - price and quantity
  - aggressor side if available from engine events
- **Basic metrics**
  - cumulative trades
  - active order count
  - total resting quantity

Format guidance:

- Prefer **NDJSON** (one object per line) for streaming-friendly parsing.
- Keep fields stable and versioned (e.g. `schema_version: 1`) for forward compatibility.

### 7A.2 UI prototype (file playback)

Build a lightweight UI that reads exported replay output files and provides:

- order book ladder (depth view)
- trade tape
- best bid / best ask + spread display
- replay controls: step forward/back, play, pause, reset
- speed control (optional)

Implementation detail intentionally flexible (React + Vite is a likely choice, but not required by this plan).

### 7A.3 Documentation

Add clear instructions for:

- generating replay visualisation output from the C++ replay path
- running the UI locally
- interpreting the views (what each panel shows)

Include an explicit note that the UI is not part of performance measurements and should not be used to claim throughput or latency.

## Acceptance criteria

- A recorded replay output file can be loaded and visualised end-to-end.
- UI remains optional and does not affect:
  - matching correctness tests
  - replay semantics
  - benchmark binaries or Release workflow
- Docs answer:
  1. what UI will be built
  2. why it exists
  3. why it is separate from the C++ core
  4. what data the C++ side must expose
  5. what the first UI prototype should show
  6. what is explicitly out of scope

## Explicitly out of scope

- Re-implementing the existing UI/export spike from scratch
- WebSocket server or HTTP server
- Live exchange connectivity
- UI embedded in C++ (Qt, Dear ImGui, etc.)
- Any changes to order matching logic
- Any changes to binary protocol semantics
- Any changes to replay semantics
- Any additional dependencies in benchmark paths

## Optional follow-ups

### 7B — Live Replay Streaming Interface

Stream the same `schemaVersion: 1` replay visualisation records as 7A file export over a **localhost-only** transport (HTTP + Server-Sent Events for browser `EventSource` compatibility). Backend streaming is implemented before UI live-follow.

| Item | Plan |
|------|------|
| Transport | HTTP/SSE on loopback (e.g. `127.0.0.1:9000`); no third-party server dependency |
| CLI | `--binary-engine <file.obk> --stream-visualisation <host:port>` (opt-in) |
| Record shape | Identical JSON fields to `--export-visualisation` NDJSON lines |
| UI | **Done** — `EventSource` live-follow in `ui/replay-visualiser/` |
| Production | Out of scope — no TLS, auth, or WAN |

See [REPLAY_VISUALISER_ROADMAP.md](REPLAY_VISUALISER_ROADMAP.md) and [REPLAY_VISUALISER.md](REPLAY_VISUALISER.md).

**Status:** **Done** (commits `5758303`, `dd1bb22`).

### 7C — UI Metrics and Benchmark Overlay

**Done.** Overlay run summary metrics in the UI (counts, totals, final BBO, spread range) derived client-side from loaded replay steps. Informational only — not Release benchmarks. See [REPLAY_VISUALISER.md](REPLAY_VISUALISER.md) §Run summary panel.

