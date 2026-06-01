# Active Milestone

**Source of truth** for `/run-active-milestone` and `/review-milestone`.

**Project root:** `cpp-low-latency-orderbook/`  
**Queue:** [MILESTONE_QUEUE.md](MILESTONE_QUEUE.md) — row **21**

**Model recommendation:** Stronger model for persistence design (journal format, recovery, equivalence). Do not start until human approves scope in a planning pass.

---

## CURRENT: 10A — Persistence / Command Journal (Revisit)

**Status:** `READY` — not started. **Queued after 9B** per [MILESTONE_8C_DECISION.md](MILESTONE_8C_DECISION.md) §6; performance track (9A/9B) complete.

### Goal

Design and implement a **durable command journal** and deterministic recovery/replay path that reproduces the same book and engine event sequence as in-memory processing — without changing matching semantics on the existing hot path until explicitly integrated.

### Background

9B concluded **no** list-node pool: mixed hash/list/map allocation on adds ([PROFILING_REPORT.md](PROFILING_REPORT.md) §9B slice 1). Infrastructure (persistence, TCP, publisher) was deferred during 8C in favour of the performance programme.

### Suggested slices (plan before code)

| # | Slice | Deliverable |
|---|--------|-------------|
| 0 | Plan doc | Journal format, append API, replay equivalence criteria |
| 1 | Append-only command log | Write path; no engine change |
| 2 | Replay from journal | `OrderCommand` stream → `MatchingEngine`; equivalence tests |
| 3 | Optional snapshot / recovery | Document tradeoffs |

### Out of scope (initial 10A)

- TCP gateway (**10B**), market data publisher (**10C**).
- Database / cloud persistence.
- Matching-engine or order-book hot-path optimisation.
- Auth, multi-user, production hardening.

### Constraints

- Preserve matching rules, FIFO, OBK1/CSV equivalence story.
- `./scripts/verify.sh` must stay **164/164** (or current count) after each slice.
- No unmeasured performance claims on the journal path.

### Acceptance criteria (sketch — refine in plan slice 0)

- [ ] Journal format documented; append and replay APIs defined.
- [ ] Replay of a recorded workload matches direct `MatchingEngine` processing (trades, book, events).
- [ ] Tests added for journal round-trip and recovery scenario.
- [ ] `./scripts/verify.sh` passes.
- [ ] Human review before `/advance-milestone`.

### Verification

```bash
./scripts/verify.sh
```

### References

- [MILESTONE_8C_DECISION.md](MILESTONE_8C_DECISION.md) §6
- [ROADMAP.md](ROADMAP.md) — Milestone 9 (persistence)
- [ARCHITECTURE.md](ARCHITECTURE.md)

### Previous milestone (9B — done)

| Deliverable | Commit |
|-------------|--------|
| Allocation profile; slice 2 not implemented (mixed evidence) | `5febd9a` |
