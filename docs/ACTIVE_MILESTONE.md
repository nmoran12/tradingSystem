# Active Milestone

**Source of truth** for `/run-active-milestone` and `/review-milestone`.

**Project root:** `cpp-low-latency-orderbook/`  
**Queue position:** See [MILESTONE_QUEUE.md](MILESTONE_QUEUE.md) — **no milestone CURRENT**

---

## Queue complete (through 7C)

The ordered implementation queue is **finished** through **7C — UI Metrics and Benchmark Overlay**.

| Verification | Result |
|--------------|--------|
| `./scripts/verify.sh` | **160/160** tests |
| `cd ui/replay-visualiser && npm run build` | Passes |
| Latest queue-advance commit | `6e23b61` — Advance milestone queue after 7C |

### Milestone 7 visualisation track (done)

| ID | Deliverable |
|----|-------------|
| 7A | NDJSON `--export-visualisation`; React UI (file/scenarios) |
| 7B | `--stream-visualisation` localhost SSE; UI live-follow |
| 7C | Run summary panel (`runMetrics.ts`) — informational only |

Do **not** start new implementation from this file until a human adds a **CURRENT** row to [MILESTONE_QUEUE.md](MILESTONE_QUEUE.md).

---

## What to do next (planning only)

1. Read **[FUTURE_IMPROVEMENTS.md](FUTURE_IMPROVEMENTS.md)** — full backlog with priority, difficulty, resume value, and **scope to avoid**.
2. Review **proposed** milestones **8A**, **8B**, **8C** in [MILESTONE_QUEUE.md](MILESTONE_QUEUE.md).
3. Agree on order (recommended: **8A → 8B → 8C**, then a systems milestone from [ROADMAP.md](ROADMAP.md)).
4. Add the chosen row as **CURRENT** and replace this file with that milestone’s goal and slices.
5. Run `/run-active-milestone` in a **new** session.

### Proposed 8A — Documentation and Demo Packaging (summary)

- **Goal:** Accurate top-level docs and demo materials so reviewers can run and understand the project quickly.
- **Includes (planned):** README/overview sync; architecture-at-a-glance; links to benchmarks; demo instructions; screenshot/GIF placeholders — **not in repo yet**.
- **Excludes:** C++ feature work, CI workflows (see 8B).

### Proposed 8B — CI and Correctness Hardening (summary)

- **Goal:** Automated build/test and stronger correctness coverage.
- **Includes (planned):** GitHub Actions; binary vs CSV equivalence; invariant workload tests; stream integration test — **not in repo yet**.
- **Excludes:** New matching semantics; perf regression gates without careful design.

### Proposed 8C — Next Systems Extension Decision (summary)

- **Goal:** Choose the next **large** systems feature before coding.
- **Candidates:** TCP order gateway · persistence/replay log · market data publisher ([ROADMAP.md](ROADMAP.md)).
- **Output:** Written decision + scoped milestone plan; **no implementation** in 8C itself.

---

## Long-term systems candidates (after 8A–8C)

| ROADMAP item | One-line summary |
|--------------|------------------|
| TCP gateway | Remote commands → `MatchingEngine` → streamed `EngineEvent`s |
| Persistence | Append-only log, snapshots, deterministic recovery replay |
| Market data publisher | Trades and BBO from engine events to subscribers |

All **candidate / not started** until queued and implemented.

---

## Scope reminder

This project should **not** expand into: full exchange platform, auth, cloud product, trading dashboard fiction, or benchmark claims without dated evidence. See [FUTURE_IMPROVEMENTS.md](FUTURE_IMPROVEMENTS.md) §Scope boundaries.
