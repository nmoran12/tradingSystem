# Active Milestone

**Source of truth** for `/run-active-milestone` and `/review-milestone`.

**Project root:** `cpp-low-latency-orderbook/`  
**Queue:** [MILESTONE_QUEUE.md](MILESTONE_QUEUE.md) — row **18**

**Model recommendation:** Default agent is sufficient for comparison docs. Use a **stronger model** when drafting detailed protocol/framing designs for the **follow-on** implementation milestone (after 8C decision).

---

## CURRENT: 8C — Next Systems Extension Decision

**Status:** `READY` — **planning only**. Do not implement TCP, persistence, publisher, or other systems code in this milestone.

### Goal

Choose and scope **one** next major systems extension before implementation begins. Produce a clear written decision recruiters and future-you can follow.

### Candidates (compare all three)

| Option | ROADMAP reference | One-line summary |
|--------|-------------------|------------------|
| **A. TCP order gateway** | Milestone 7 (systems) | Remote framed commands → `MatchingEngine` → streamed `EngineEvent`s; gateway module, engine stays socket-free |
| **B. Persistence / replay log** | Milestone 9 | Append-only command log, snapshots, crash-recovery replay with identical book/event sequence |
| **C. Market data publisher** | Milestone 8 | In-process subscriber for trades and BBO (optional depth) from `EngineEvent`s |

**Sequencing note** ([ROADMAP.md](ROADMAP.md)): commands-before-market-data (7→8) and durability-before-replication (9→10). Your decision should state whether you follow that order or deviate and why.

### Slices (documentation deliverables)

#### 1. Comparison matrix

For each candidate, document:

- Problem solved / portfolio story
- Dependencies on existing code (binary OBK1, SPSC, viz stream, CI)
- Estimated complexity (S/M/L) and test strategy
- Risks and out-of-scope boundaries (no TLS/auth/cloud in v1)
- Resume / interview talking points

#### 2. Recommendation

- **Pick one** primary next implementation milestone (A, B, or C).
- **Defer** the others with one-sentence rationale each.
- Optional: ordered “phase 2 / phase 3” if you want a multi-quarter roadmap without implementing now.

#### 3. Scoped implementation outline (for the chosen option only)

- Module boundaries (what stays out of `MatchingEngine` / `OrderBook`)
- 3–6 implementation slices for the **next** queue row (not built in 8C)
- Acceptance criteria sketch for that future milestone
- What **not** to build in v1

**Suggested artifact:** `docs/MILESTONE_8C_DECISION.md` (create in `/run-active-milestone`).

### Out of scope (8C)

- Any C++, CMake, test, or CI workflow changes (unless fixing a doc typo).
- Implementing TCP gateway, persistence, market data publisher, auth, databases, or cloud deploy.
- Changing matching semantics or benchmark claims.
- Advancing the queue to a systems implementation row (human adds that row after reviewing 8C output).

### Constraints

- Be honest: label anything not in the repo as **planned**.
- Do not overclaim production readiness.
- Keep `profiling/` out of commits.

### Acceptance criteria

- [ ] Comparison matrix covers all three candidates (TCP, persistence, publisher).
- [ ] Single **recommended** next systems milestone with rationale.
- [ ] Scoped slice outline for the recommended option (for a future implementation milestone).
- [ ] Explicit out-of-scope list for v1 of the chosen path.
- [ ] `docs/MILESTONE_QUEUE.md` unchanged for implementation status until human queues the next row.
- [ ] `./scripts/verify.sh` still passes if any doc-only edits were made (**164** tests).
- [ ] Human review + `/review-milestone` before queuing the implementation milestone.

### Verification

```bash
./scripts/verify.sh
```

No UI or benchmark run required for pure planning docs.

### References

- [ROADMAP.md](ROADMAP.md) — systems milestones 7–10
- [FUTURE_IMPROVEMENTS.md](FUTURE_IMPROVEMENTS.md) — backlog context
- [ARCHITECTURE.md](ARCHITECTURE.md) — module boundaries
- Prior milestone (done): **8B** — `266df9d`, CI + correctness tests

### Previous milestone (8B — done)

| Deliverable | Commit |
|-------------|--------|
| GitHub Actions + 4 new tests (164 total) | `266df9d` |
