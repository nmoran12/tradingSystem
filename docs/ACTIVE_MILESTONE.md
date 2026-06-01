# Active Milestone

**Source of truth** for `/run-active-milestone` and `/review-milestone`.

**Project root:** `cpp-low-latency-orderbook/`  
**Queue position:** See [MILESTONE_QUEUE.md](MILESTONE_QUEUE.md) — **7A is CURRENT**

---

## Milestone 7A — Replay Visualiser UI

| Field | Value |
|-------|--------|
| **Status** | IN PROGRESS (spike landed; closing slice: tests + doc sync) |
| **Parent** | Milestone 7 — Optional visualisation (offline-first) |
| **Test baseline** | 149 tests after 6H; export CLI tests added in current slice |

### What already exists (do not reimplement)

| Deliverable | Location / notes |
|-------------|------------------|
| NDJSON export | Inline in `src/main.cpp` (`write_visualisation_step`) |
| CLI | `--binary-engine <file.obk> --export-visualisation <replay.ndjson>` only |
| UI spike | `ui/replay-visualiser/` (React + Vite) |
| Docs | [REPLAY_VISUALISER.md](REPLAY_VISUALISER.md) |

**Not implemented:** `--export-viz` alias; export on `--engine` (CSV) or `--replay`; full depth in export (shallow BBO level per side only).

### Current slice (this change set)

- Automated CLI/shape tests for binary-engine visualisation export (`tests/test_cli_visualisation_export.cpp`)
- Milestone doc reconciliation (this file, [MILESTONE_7_PLAN.md](MILESTONE_7_PLAN.md), [REPLAY_VISUALISER.md](REPLAY_VISUALISER.md))
- **No** UI rewrite, exporter refactor, or export semantics changes unless a test exposes a bug

### Goal

Optional replay visualiser for debugging and learning — **without** changing matching semantics, replay semantics, or Release benchmark hot paths. C++ core stays headless; UI consumes file-based NDJSON offline.

### Scope (remaining for 7A close)

- [x] Opt-in NDJSON export (`schemaVersion: 1`) from `--binary-engine`
- [x] React + Vite UI prototype (ladder, tape, BBO, playback, chart)
- [x] [REPLAY_VISUALISER.md](REPLAY_VISUALISER.md)
- [x] CLI export tests (shape + trade line)
- [ ] Human review; optional: regenerate one UI sample from CLI export vs rich demo fixtures
- [ ] Advance queue after `/review-milestone` (human approval)

### Out of scope

- Live streaming (**7B**), benchmark overlay UI (**7C**)
- CSV/market-event export paths, full book depth export, `--export-viz` alias
- Networking, persistence, matching/binary protocol changes
- SPSC / performance work (completed under **6H**)

### Acceptance criteria

- [x] `./scripts/verify.sh` passes (including new export tests)
- [x] Export mode opt-in; default CLI unchanged
- [x] NDJSON producible from deterministic tiny `.obk` (tests generate in temp dir)
- [x] Docs separate UI from C++ core and benchmarks
- [x] No UI linked into `orderbook_core` or benchmark targets

### Required verification

```bash
./scripts/verify.sh
```

Manual export example (generate `.obk` via tests/workload or binary writer; repo ships CSV only under `data/`):

```bash
./build/cpp-low-latency-orderbook \
  --binary-engine /path/to/commands.obk \
  --export-visualisation /tmp/replay.ndjson
```

### Key docs

- [MILESTONE_7_PLAN.md](MILESTONE_7_PLAN.md)
- [REPLAY_VISUALISER.md](REPLAY_VISUALISER.md)
- [ARCHITECTURE.md](ARCHITECTURE.md)
- [BENCHMARKING.md](BENCHMARKING.md)

### Human review before advance

Run `/review-milestone`, then human approval before `/advance-milestone`.
