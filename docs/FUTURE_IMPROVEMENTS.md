# Future Improvements and Backlog

**Status:** Planning document only. Items below are **candidates** unless explicitly marked as shipped elsewhere. Nothing in this file is implemented unless the repo already contains it.

**Current baseline (post-9B):** `./scripts/verify.sh` — **164/164** tests; queue **10A CURRENT** ([ACTIVE_MILESTONE.md](ACTIVE_MILESTONE.md)).

**Related:** [ROADMAP.md](ROADMAP.md) (long-term systems milestones), [MILESTONE_QUEUE.md](MILESTONE_QUEUE.md) (proposed **8A–8C**), [ACTIVE_MILESTONE.md](ACTIVE_MILESTONE.md).

---

## Scope boundaries (explicitly avoided)

This project is **not** becoming:

- A full exchange or brokerage platform
- An auth/accounts or multi-user product
- A cloud-hosted trading service (unless you fork it separately)
- A strategy, PnL, or portfolio system
- A fake “production dashboard” with unsubstantiated latency claims

Also **avoid** unless profiler evidence justifies it:

- Order-book container rewrites (`std::list` / `std::map` family swaps) — rejected after 6G measurement
- Multithreaded SPSC “optimisation” without measured win — 6H pipeline was slower single-threaded
- Committing `profiling/` trace artifacts (keep local; gitignored)
- Benchmark throughput claims without dated, reproducible Release runs ([BENCHMARKING.md](BENCHMARKING.md))

---

## Backlog index

| Item | Category | Priority | Difficulty | Resume |
|------|----------|----------|------------|--------|
| [README / overview refresh](#documentation--readme-polish) | Docs | **now** | small | high |
| [Architecture-at-a-glance in README](#documentation--readme-polish) | Docs | now | small | high |
| [Pinned benchmark summary table](#benchmarking--profiling) | Docs | now | small | medium |
| [GitHub Actions: build + ctest + UI build](#ci-and-testing) | CI | **now** | small | high |
| [README screenshot / demo GIF](#demo-packaging) | Demo | now | small | high |
| [Live replay demo script](#demo-packaging) | Demo | now | small | high |
| [Committed golden `.obk` or generator one-liner](#demo-packaging) | Demo | now | small | medium |
| [Binary vs CSV equivalence test](#ci-and-testing) | CI | later | medium | high |
| [Stable stream CLI integration test](#ci-and-testing) | CI | later | medium | medium |
| [OBK1 decode fuzz / property tests](#ci-and-testing) | CI | later | medium | medium |
| [Long-workload invariant tests](#ci-and-testing) | CI | later | small | medium |
| [CI Release benchmark smoke (bounded)](#benchmarking--profiling) | CI | later | medium | high |
| [Regenerate UI fixtures from CLI export](#replay-visualiser-improvements) | UI | later | small | medium |
| [Full-depth / top-N visualisation export](#replay-visualiser-improvements) | Core/UI | later | medium | medium |
| [Metrics at current step toggle](#replay-visualiser-improvements) | UI | later | small | low |
| [Playback speed control](#replay-visualiser-improvements) | UI | later | small | low |
| [Live stream reconnect / demo polish](#replay-visualiser-improvements) | UI | later | small | low |
| [TCP order gateway](#future-systems-milestones) | Systems | later | large | high |
| [Persistence / replay log](#future-systems-milestones) | Systems | later | large | high |
| [Market data publisher](#future-systems-milestones) | Systems | later | large | medium |
| [Multithreaded SPSC without evidence](#explicitly-avoided-scope-creep) | Perf | **avoid** | large | — |
| [Container/order-book rewrite](#explicitly-avoided-scope-creep) | Perf | **avoid** | large | — |
| [Huge UI rewrite / benchmark ingestion UI](#explicitly-avoided-scope-creep) | UI | **avoid** | large | — |

---

## Documentation / README polish

**8A:** README refresh, architecture-at-a-glance, and [DEMO_GUIDE.md](DEMO_GUIDE.md) are **done**; screenshot/GIF **checklist** is in the demo guide (binary assets still optional).

**9A/9B (done):** Performance plan (`191ddab`); alloc profile (`5febd9a`) — **no** list pool (mixed evidence). **10A (CURRENT):** persistence / command journal revisit per [MILESTONE_8C_DECISION.md](MILESTONE_8C_DECISION.md).

### README / overview refresh

- **Description:** Sync `README.md`, [PROJECT_OVERVIEW.md](PROJECT_OVERVIEW.md), and top-level status lines with **160** tests, completed milestones **5–7C**, binary OBK1, SPSC, visualiser, `./scripts/verify.sh`.
- **Status:** Addressed in milestone **8A** (see [DEMO_GUIDE.md](DEMO_GUIDE.md)).
- **Why:** Stale “123 tests” / “Milestones 1–6 in progress” undermines trust for reviewers.
- **Priority:** now · **Difficulty:** small · **Resume:** high

### Architecture-at-a-glance in README

- **Description:** Single diagram: replay path, engine CSV path, binary engine path, optional viz export/stream, UI (file + live), benchmarks off to the side.
- **Why:** Recruiters skim README; [ARCHITECTURE.md](ARCHITECTURE.md) is detailed but dated in places.
- **Priority:** now · **Difficulty:** small · **Resume:** high

---

## CI and testing

**8B (CURRENT):** GitHub Actions, CSV/binary equivalence, workload invariants, and workload SSE stream test are **implemented** — see [`.github/workflows/ci.yml`](../.github/workflows/ci.yml) and `tests/test_csv_binary_engine_equivalence.cpp`.

### GitHub Actions: build + ctest + UI build

- **Description:** Workflow on push/PR: `cmake` Release or Debug build, `ctest`, optional `cd ui/replay-visualiser && npm run build`.
- **Status:** **Shipped in 8B** (Ubuntu Debug; no benchmark gates).
- **Why:** Largest credibility gap vs a portfolio C++ repo.
- **Priority:** now · **Difficulty:** small · **Resume:** high

### Binary vs CSV equivalence test

- **Description:** Same command sequence via CSV `--engine` and binary `--binary-engine` → equivalent final book / trade counts.
- **Status:** **Shipped in 8B** (`test_csv_binary_engine_equivalence.cpp`).
- **Why:** Strong correctness signal for OBK1 path.
- **Priority:** later · **Difficulty:** medium · **Resume:** high

### Stable stream CLI integration test

- **Description:** Reliable test for `--stream-visualisation` (subprocess was removed as flaky); in-process or scripted client.
- **Why:** Closes 7B coverage gap.
- **Priority:** later · **Difficulty:** medium · **Resume:** medium

### OBK1 decode fuzz / property tests

- **Description:** Random/corrupt 64-byte messages; decoder must reject safely.
- **Why:** Wire-format robustness story.
- **Priority:** later · **Difficulty:** medium · **Resume:** medium

### Long-workload invariant tests

- **Description:** Call `OrderBook::validate_invariants()` after large `WorkloadGenerator` or SPSC drain runs.
- **Status:** **Shipped in 8B** (2 000-command matching-engine test; SPSC workload tests already had invariants).
- **Why:** Cheap extension of existing introspection.
- **Priority:** later · **Difficulty:** small · **Resume:** medium

---

## Demo packaging

### README screenshot / demo GIF

- **Description:** Static image or short GIF: ladder + trade tape + run summary (+ optional live connect). **Not in repo yet.**
- **Why:** Makes 7A–7C tangible in seconds on GitHub.
- **Priority:** now · **Difficulty:** small · **Resume:** high

### Live replay demo script

- **Description:** `scripts/demo-live-replay.sh` documenting start order: C++ `--stream-visualisation`, then UI `npm run dev`, Connect. **Not yet implemented.**
- **Why:** Two-terminal flow is easy to get wrong ([REPLAY_VISUALISER.md](REPLAY_VISUALISER.md)).
- **Priority:** now · **Difficulty:** small · **Resume:** high

### Committed golden `.obk` or generator one-liner

- **Description:** Small committed sample under `data/` (if policy allows) or documented one-liner using test writer; `*.obk` may stay gitignored.
- **Why:** Faster onboarding than “run tests to generate.”
- **Priority:** now · **Difficulty:** small · **Resume:** medium

---

## Benchmarking / profiling

### Pinned benchmark summary table

- **Description:** One dated table in README or [PERFORMANCE_BASELINE.md](PERFORMANCE_BASELINE.md) (machine, seed, cmd/s) with “sample only” disclaimer.
- **Why:** Shows discipline without overclaiming; complements `repeated-benchmark.sh`.
- **Priority:** now · **Difficulty:** small · **Resume:** medium

### CI Release benchmark smoke (bounded)

- **Description:** Optional CI job: small N (e.g. 10k commands), fail only on crash or invariant break — not perf regression gates.
- **Why:** Catches Release-only breakage; avoid flaky perf thresholds.
- **Priority:** later · **Difficulty:** medium · **Resume:** high

---

## Replay visualiser improvements

All **not yet implemented** unless already shipped in 7A–7C.

### Regenerate UI fixtures from CLI export

- **Description:** Align `ui/replay-visualiser/public/*.ndjson` with `--export-visualisation` output (BBO depth).
- **Why:** Bundled multi-level fixtures exceed C++ exporter today.
- **Priority:** later · **Difficulty:** small · **Resume:** medium

### Full-depth / top-N visualisation export

- **Description:** C++ exporter emits more than one level per side; schema bump only if required.
- **Why:** UI ladder demo vs export truth.
- **Priority:** later · **Difficulty:** medium · **Resume:** medium

### Metrics at current step toggle

- **Description:** Run summary for full replay vs metrics at scrub index (7C follow-up).
- **Why:** Clarifies inspection while scrubbing live stream.
- **Priority:** later · **Difficulty:** small · **Resume:** low

### Playback speed control

- **Description:** Adjustable play interval in UI (7A optional).
- **Why:** Demo polish.
- **Priority:** later · **Difficulty:** small · **Resume:** low

### Live stream reconnect / demo polish

- **Description:** Reconnect, clear-steps, production SSE — defer per 7B PoC scope.
- **Why:** Nice-to-have; not required for milestone close.
- **Priority:** later · **Difficulty:** small–medium · **Resume:** low

---

## Core matching engine / correctness improvements

Covered mainly under [CI and testing](#ci-and-testing). No matching-logic changes planned without tests.

---

## Future systems milestones

**8C decision (done, revised):** [MILESTONE_8C_DECISION.md](MILESTONE_8C_DECISION.md) recommends **9A performance deep dive** (plan) and **9B** (one measured optimisation if justified). **Persistence, TCP, and publisher deferred** to 10A+. **Not implemented** until queued.

Documented in depth in [ROADMAP.md](ROADMAP.md).

### TCP order gateway

- **Description:** Separate gateway module: TCP → framed messages → `OrderCommand` → `EngineEvent` stream; engine stays socket-free.
- **Why:** Natural “exchange infrastructure” step after binary + optional SPSC.
- **Priority:** later · **Difficulty:** large · **Resume:** high

### Persistence / replay log

- **Description:** Append-only command log, snapshots, crash-recovery replay with identical book/event sequence.
- **Why:** Durability and determinism story; enables replication research later.
- **Priority:** later · **Difficulty:** large · **Resume:** high

### Market data publisher

- **Description:** In-process subscriber for trades and BBO updates from `EngineEvent`s.
- **Why:** Completes command-in / market-data-out narrative.
- **Priority:** later · **Difficulty:** large · **Resume:** medium

---

## Explicitly avoided scope creep

See [Scope boundaries](#scope-boundaries-explicitly-avoided) above.

---

## Proposed milestone packaging

Human-readable grouping for [MILESTONE_QUEUE.md](MILESTONE_QUEUE.md) — **Proposed**, not CURRENT:

| ID | Name | Goal |
|----|------|------|
| **8A** | Documentation and Demo Packaging | README accuracy, architecture-at-a-glance, demo instructions, screenshot/GIF placeholders |
| **8B** | CI and Correctness Hardening | GitHub Actions, equivalence tests, invariants, stream coverage |
| **8C** | Next Systems Extension Decision | Plan TCP vs persistence vs market data publisher before coding |

Implement **8A → 8B → 8C** (or re-order after review) before starting a large systems milestone.
