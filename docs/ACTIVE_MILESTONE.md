# Active Milestone

**Source of truth** for `/run-active-milestone` and `/review-milestone`.

**Project root:** `cpp-low-latency-orderbook/`  
**Queue:** [MILESTONE_QUEUE.md](MILESTONE_QUEUE.md) — row **19**

**Model recommendation:** Default agent for measurement and docs. Use careful, evidence-based language; no optimisation code until **9B** and only if 9A justifies a target.

---

## CURRENT: 9A — Performance Deep Dive and Hot-Path Optimisation Plan

**Status:** **Complete (pending human review)** — planning and measurement only; **no** engine/book/protocol code changes in 9A.

**Deliverable:** [MILESTONE_9A_PERFORMANCE_PLAN.md](MILESTONE_9A_PERFORMANCE_PLAN.md)

### Goal

Produce an evidence-based picture of where time goes today and a **scoped, low-risk optimisation plan** for the matching engine, order book, and replay/benchmark paths — without claiming improvements that have not been measured.

### Background (8C decision)

[MILESTONE_8C_DECISION.md](MILESTONE_8C_DECISION.md) chose **performance-first** over persistence, TCP gateway, and market data publisher. Infrastructure remains **deferred**.

### Slices

| # | Slice | Status |
|---|--------|--------|
| 1 | Clean Release baselines | Done — [MILESTONE_9A_PERFORMANCE_PLAN.md](MILESTONE_9A_PERFORMANCE_PLAN.md) §2, [PERFORMANCE_BASELINE.md](PERFORMANCE_BASELINE.md) §9A |
| 2 | Re-profile | Done — 6G apply-loop evidence + 9A session notes ([PROFILING_REPORT.md](PROFILING_REPORT.md) §9A) |
| 3 | Hotspot ranking | Done — plan §4 |
| 4 | Cost separation | Done — plan §3 |
| 5 | Data-structure review | Done — plan §5; no rewrite |
| 6 | Optimisation candidates | Done — plan §6 |
| 7 | 9B plan | Done — plan §7–9 (**recommended 9B:** list node alloc, conditional on byte-ranked profile) |

### Out of scope (9A)

- Implementing optimisations in engine/book (→ **9B**, conditional).
- Persistence / command journal, TCP gateway, market data publisher.
- `std::list` / `std::map` wholesale replacement without profiler proof (6G lesson).
- CI benchmark regression gates.
- Committing `profiling/` trace bundles.
- UI, matching-semantics, or protocol layout changes.

### Acceptance criteria

- [x] Dated Release baseline table in docs.
- [x] Profiler section for post-8B `HEAD`.
- [x] Hotspot list with engine vs non-engine split.
- [x] Written 9B recommendation tied to profiler evidence.
- [x] No matching-semantics changes in 9A.
- [x] `./scripts/verify.sh` passes (**164/164**).
- [ ] Human review + `/review-milestone` before `/advance-milestone` to **9B**.

### Verification

```bash
./scripts/verify.sh
```

### References

- [MILESTONE_8C_DECISION.md](MILESTONE_8C_DECISION.md) §4–5
- [MILESTONE_9A_PERFORMANCE_PLAN.md](MILESTONE_9A_PERFORMANCE_PLAN.md)
- [BENCHMARKING.md](BENCHMARKING.md) · [PERFORMANCE_BASELINE.md](PERFORMANCE_BASELINE.md) · [PERFORMANCE_ROADMAP.md](PERFORMANCE_ROADMAP.md) · [PROFILING_REPORT.md](PROFILING_REPORT.md)

### Previous milestone (8C — done)

| Deliverable | Commit |
|-------------|--------|
| Performance-first roadmap decision | `e85786b` |
