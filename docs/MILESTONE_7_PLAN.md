# Milestone 7 Plan: Replay Visualiser UI

**Status:** 7A **Done**; **7B CURRENT** (see [MILESTONE_QUEUE.md](MILESTONE_QUEUE.md), [ACTIVE_MILESTONE.md](ACTIVE_MILESTONE.md)).  
**Prerequisite:** Replay and engine paths complete; no performance work is blocked on this milestone.

### Implementation snapshot (repo)

| Item | Status |
|------|--------|
| 7A.1 NDJSON export | **Done** — `--binary-engine` + `--export-visualisation`; shallow BBO depth only (`src/main.cpp`) |
| 7A.2 UI prototype | **Done** — `ui/replay-visualiser/` (React + Vite) |
| 7A.3 Documentation | **Done** — [REPLAY_VISUALISER.md](REPLAY_VISUALISER.md) |
| CLI export tests | **Done** — `tests/test_cli_visualisation_export.cpp` |
| 7A follow-ups (optional) | CSV/`--replay` export; full depth; `--export-viz` alias; UI tests; CLI-aligned fixtures |
| 7B streaming | **Queued → CURRENT** — see [ACTIVE_MILESTONE.md](ACTIVE_MILESTONE.md) |

## Goal

Add an optional UI replay visualiser that helps users understand and debug order book behaviour **without** adding dependencies, instrumentation overhead, or behaviour changes to the performance-critical C++ core.

The UI is for education, debugging, and demo purposes. It is not part of the low-latency benchmark path.

## Architecture principles

- **Core remains headless:** `OrderBook` and `MatchingEngine` do not import or depend on any UI/frontend code.
- **Deterministic exports:** The C++ side exports replay outputs deterministically for a given input and seed.
- **Offline-first:** Start with file-based playback (JSON or NDJSON). Live streaming is a follow-up milestone.
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

Stream NDJSON over a local interface (still non-production) so the UI can follow a replay as it runs.

### 7C — UI Metrics and Benchmark Overlay

Overlay run metadata and summary metrics in the UI (counts, totals, and run configuration). This must remain informational and not replace Release benchmarks.

