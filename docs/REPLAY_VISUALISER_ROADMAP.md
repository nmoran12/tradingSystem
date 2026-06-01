# Replay Visualiser Roadmap

This document tracks the optional replay visualisation milestone slices. It complements [REPLAY_VISUALISER.md](REPLAY_VISUALISER.md) (how to run what exists today) and [MILESTONE_7_PLAN.md](MILESTONE_7_PLAN.md) (acceptance and architecture).

## Milestone slices

| Slice | Name | Status | Summary |
|-------|------|--------|---------|
| **7A** | File-based replay visualisation | **Done** | NDJSON export from `--binary-engine`; React/Vite UI loads files offline |
| **7B** | Live replay streaming | **Done (PoC)** | Localhost SSE CLI + UI `EventSource` live-follow |
| **7C** | UI metrics / benchmark overlay | **Done** | Run summary panel; client-side aggregation only |

## 7A — File-based (complete)

- **Backend:** `--binary-engine <file.obk> --export-visualisation <replay.ndjson>`
- **Format:** One JSON object per line (NDJSON); `schemaVersion: 1`
- **UI:** `ui/replay-visualiser/` — load file, step/play controls
- **Depth:** Shallow BBO export only (one aggregated level per side at best price)

## 7B — Live replay streaming

### Goal

Stream the **same** replay visualisation record shape as 7A file export while the binary engine processes commands, so the UI can later **follow playback live** without re-exporting.

### Principles

- **Same schema:** `schemaVersion: 1` JSON fields match file export (no new schema unless explicitly versioned later).
- **Backend first:** C++ localhost SSE proof-of-concept before UI live-follow.
- **Opt-in:** New CLI flag only; default `--binary-engine` behaviour unchanged.
- **Localhost-only:** Bind restricted to loopback (e.g. `127.0.0.1:9000`); no TLS, auth, or WAN.
- **Non-production:** Blocking single-client server; not for trading or benchmark claims.
- **No hot-path coupling:** No changes to `MatchingEngine` / `OrderBook` matching logic; no instrumentation in `matching_engine_benchmark` or `binary_protocol_benchmark`.

### CLI (backend slice — shipped)

```bash
./build/cpp-low-latency-orderbook \
  --binary-engine /path/to/order_commands.obk \
  --stream-visualisation 127.0.0.1:9000
```

Implementation: `viz::ReplayVisualisationWriter` (shared JSON) + `viz::ReplayVisualisationStreamServer` (POSIX HTTP/SSE). Client connects (e.g. `curl -N`); server emits one SSE `data:` frame per engine command step.

### UI live-follow (slice 2 — shipped)

- `ui/replay-visualiser/` — Connect / Disconnect, default `http://127.0.0.1:9000/stream`
- Appends steps by `index`; follows newest step by default

### Optional polish (later)

- WebSocket transport
- Reconnect/backpressure
- Small demo script documenting backend + UI startup order
- Production gateway patterns (see [ROADMAP.md](ROADMAP.md) Milestone 7 TCP — separate track)

## 7C — UI metrics overlay (complete)

Run summary panel aggregates command/trade totals, final book snapshot, and min/max spread from all loaded steps (file, scenario, or live stream). Informational only; does not replace Release benchmarks. Commit `7423903`.

## Out of scope (all visualisation milestones)

- Changing matching or replay semantics
- Embedding UI in C++
- Live exchange connectivity
- Benchmark hot-path instrumentation by default
