# Active Milestone

**Source of truth** for `/run-active-milestone` and `/review-milestone`.

**Project root:** `cpp-low-latency-orderbook/`  
**Queue position:** See [MILESTONE_QUEUE.md](MILESTONE_QUEUE.md) — **7A is CURRENT**

---

## Milestone 7A — Replay Visualiser UI

| Field | Value |
|-------|--------|
| **Status** | READY (workflow) |
| **Parent** | Milestone 7 — Optional visualisation (offline-first) |
| **Test baseline** | 149 tests after 6H SPSC pipeline work |

### Model recommendation

Use a **clear, product-minded model** for export schema and UI structure. The C++ export path must stay simple and gated; avoid coupling `OrderBook` / `MatchingEngine` to frontend code. **Cursor Auto** is reasonable for a minimal NDJSON export slice and docs; prefer a **stronger model** for full UI layout, playback state, and schema versioning if the first slice grows quickly.

---

### Goal

Build an **optional** replay visualiser so users can understand and debug order-book behaviour from recorded output — **without** changing matching semantics, replay semantics, or Release benchmark hot paths.

The C++ core stays headless. Visualisation data is exported deterministically (file-based first), then consumed by a separate UI (e.g. React + Vite under a `viz/` or similar folder — implementation detail flexible).

### First slice (planned)

1. **Design and implement a minimal offline export path** (engine or replay run) that writes versioned **NDJSON** (or JSON lines) with stable fields, for example:
   - step index, optional timestamp
   - best bid / best ask / spread
   - trades (price, quantity, sides/order ids where available)
   - optional shallow depth or top-of-book only for v1
   - summary counters (active orders, resting quantity) per step or at end
2. **Explicit CLI or mode** (e.g. `--export-viz <file.ndjson>`) — not enabled in benchmark binaries by default.
3. **Docs** — how to generate a sample file and what each field means; state that the UI is not part of performance measurement.
4. **Do not** in the first slice:
   - embed UI in C++ (Qt, ImGui, etc.)
   - add WebSocket/HTTP server (that is **7B**)
   - change `MatchingEngine` / `OrderBook` matching logic
   - instrument `matching_engine_benchmark` or `binary_protocol_benchmark` hot loops
   - claim throughput or latency improvements from the visualiser

Later slices: minimal UI prototype reading the export file (ladder, tape, BBO, step/play controls) — see [MILESTONE_7_PLAN.md](MILESTONE_7_PLAN.md) §7A.2.

### Likely files touched (first slice)

| Area | Examples |
|------|----------|
| Export / orchestration | `src/main.cpp`, new `include/viz/` or `src/viz/` exporter (thin) |
| Types / schema | Small header for export record shapes; `schema_version` field |
| Tests | Golden-file or round-trip tests on export lines from a tiny fixed command sequence |
| Docs | `docs/MILESTONE_7_PLAN.md`, new `docs/VISUALISER.md` or section in README |
| UI (later slice) | `viz/` frontend (not required in first slice) |

### Scope

- Deterministic, file-based visualisation export from an existing engine or replay path
- Stable, documented field names and schema version
- No regression to existing CLI modes (`--replay`, `--engine`, `--binary-engine`)
- Keep benchmarks and default builds free of export overhead

### Out of scope

- Live streaming interface (**7B**)
- Benchmark overlay in UI (**7C**)
- TCP gateway, persistence, replication
- Binary protocol semantic changes
- Matching or FIFO / price-time rule changes
- SPSC pipeline or performance optimisation work (completed under **6H**)

### Acceptance criteria

- [ ] `./scripts/verify.sh` passes (add tests if export behaviour is new)
- [ ] Export mode is opt-in and documented; default behaviour unchanged
- [ ] Sample NDJSON (or documented format) can be produced from a small deterministic run
- [ ] Docs explain separation from C++ core and from Release benchmarks
- [ ] No UI dependency linked into `orderbook_core` or benchmark targets in the first slice unless explicitly approved
- [ ] No later-milestone work (7B/7C) in the same change set unless explicitly approved

### Required verification

From project root:

```bash
./scripts/verify.sh
```

Manual check after export slice exists:

```bash
# Example — exact flags TBD when implemented
./build/cpp-low-latency-orderbook --engine data/sample_commands.csv --export-viz /tmp/replay.ndjson
```

### Key docs

- [MILESTONE_7_PLAN.md](MILESTONE_7_PLAN.md)
- [ARCHITECTURE.md](ARCHITECTURE.md)
- [DEVELOPMENT_RULES.md](DEVELOPMENT_RULES.md)
- [BENCHMARKING.md](BENCHMARKING.md) — UI must not pollute benchmark paths

### Human review before advance

After `/run-active-milestone` completes, run `/review-milestone`, then human approval before `/advance-milestone`.
