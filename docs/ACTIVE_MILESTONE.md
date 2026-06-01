# Active Milestone

**Source of truth** for `/run-active-milestone` and `/review-milestone`.

**Project root:** `cpp-low-latency-orderbook/`  
**Queue:** [MILESTONE_QUEUE.md](MILESTONE_QUEUE.md) — row **16**

---

## CURRENT: 8A — Documentation and Demo Packaging

**Status:** Queued for implementation — **not started** in this milestone tranche (queue prep only until `/run-active-milestone`).

### Goal

Make the project easy to understand, run, and assess from GitHub.

### Slices

#### 1. README final polish

- Accurate **160**-test baseline (`./scripts/verify.sh`).
- Concise **“what I built”** section (matching engine, binary OBK1, benchmarks, optional replay visualiser).
- **Architecture-at-a-glance** (replay path, CSV engine path, binary engine path, optional viz export/stream, UI, benchmarks off to the side).
- **Benchmark / profiling** links with honest disclaimers ([BENCHMARKING.md](BENCHMARKING.md), [PERFORMANCE_BASELINE.md](PERFORMANCE_BASELINE.md), [PROFILING_REPORT.md](PROFILING_REPORT.md)) — machine-local, dated evidence only.
- **Replay visualiser** feature summary: file/scenario load, live SSE, run summary metrics ([REPLAY_VISUALISER.md](REPLAY_VISUALISER.md)).

#### 2. Demo packaging

- Document how to run **offline** visualiser scenarios (bundled NDJSON / upload).
- Document how to run **live SSE** visualisation (`--stream-visualisation` + UI connect) — see [REPLAY_VISUALISER.md](REPLAY_VISUALISER.md).
- Add a **placeholder checklist** for README screenshot / demo GIF capture (paths and steps; assets may remain absent until captured).
- **Optional future:** `scripts/demo-live-replay.sh` — note in docs/backlog only; **do not implement** unless explicitly approved in a later slice.

#### 3. Onboarding polish

- One clear path: **clone → build → test → run demo** (C++ verify, optional UI build, sample replay/engine commands, visualiser).
- State that **`profiling/`** trace directories are local-only and must **not** be committed.
- Ensure docs do **not** overclaim benchmark wins; point readers to repeat scripts and baseline tables.

### Constraints

- **Docs only** for 8A — no matching-engine, order-book, parser, or protocol behaviour changes.
- **No** C++ or React/UI source changes unless a future slice explicitly allows doc-adjacent fixes (default: **none**).
- **No** GitHub Actions or other CI workflows (deferred to **8B**).
- **No** `scripts/demo-live-replay.sh` unless human approves within 8A.
- **No** screenshot/GIF binary assets required to mark 8A done — checklist and instructions are enough if assets are still pending capture.
- **No** new benchmark throughput claims without dated, reproducible Release runs on this repo.

### Acceptance criteria

- [ ] `README.md` reflects post-7C reality (160 tests, milestones through 7C, architecture-at-a-glance, honest perf disclaimers, visualiser summary).
- [ ] Demo sections cover offline scenarios and live SSE end-to-end (commands + UI steps).
- [ ] Screenshot/GIF **capture checklist** exists (placeholders OK; no false claim that images are in-repo).
- [ ] Onboarding path is a single obvious flow for a new clone.
- [ ] `profiling/` called out as untracked/local-only.
- [ ] `./scripts/verify.sh` still passes after any doc-only edits.
- [ ] Human review + `/review-milestone` before `/advance-milestone` to **8B**.

### References

- Backlog detail: [FUTURE_IMPROVEMENTS.md](FUTURE_IMPROVEMENTS.md)
- Visualiser: [REPLAY_VISUALISER.md](REPLAY_VISUALISER.md), [REPLAY_VISUALISER_ROADMAP.md](REPLAY_VISUALISER_ROADMAP.md)
- Long-term systems (not 8A): [ROADMAP.md](ROADMAP.md)

### Baseline verification (pre-8A)

| Check | Result |
|-------|--------|
| `./scripts/verify.sh` | **160/160** |
| `cd ui/replay-visualiser && npm run build` | Passes |
| Post-7C docs commit | `378c3cb` |
