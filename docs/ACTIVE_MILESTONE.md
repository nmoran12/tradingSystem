# Active Milestone

**Source of truth** for `/run-active-milestone` and `/review-milestone`.

**Project root:** `cpp-low-latency-orderbook/`  
**Queue:** [MILESTONE_QUEUE.md](MILESTONE_QUEUE.md) — row **19**

**Model recommendation:** Default agent for measurement and docs. Use careful, evidence-based language; no optimisation code until **9B** and only if 9A justifies a target.

---

## CURRENT: 9A — Performance Deep Dive and Hot-Path Optimisation Plan

**Status:** `READY` — not started. **Planning and measurement only** in 9A; do not implement hot-path code changes in this milestone unless explicitly moved to 9B.

### Goal

Produce an evidence-based picture of where time goes today and a **scoped, low-risk optimisation plan** for the matching engine, order book, and replay/benchmark paths — without claiming improvements that have not been measured.

### Background (8C decision)

[MILESTONE_8C_DECISION.md](MILESTONE_8C_DECISION.md) chose **performance-first** over persistence, TCP gateway, and market data publisher. Infrastructure remains **deferred**.

### Slices

| # | Slice | Deliverable |
|---|--------|-------------|
| 1 | Clean Release baselines | Re-run `./scripts/benchmark_release.sh` / `./scripts/benchmark_repeat.sh`; **dated** table (machine-local labels) |
| 2 | Re-profile | Update [PROFILING_REPORT.md](PROFILING_REPORT.md) for current `HEAD` — Instruments on engine/benchmark hot path |
| 3 | Hotspot ranking | Top functions with % time; engine-only vs parse/replay/viz |
| 4 | Cost separation | Document which subsystems dominate (engine loop, binary path, viz, SPSC) |
| 5 | Data-structure review | `OrderBook` / `MatchingEngine` allocation and traversal notes — **no rewrite without evidence** |
| 6 | Optimisation candidates | Rank 2–4 candidates; select **0–2** for 9B with go/no-go criteria |
| 7 | 9B plan | Short doc: target, metrics, tests, rollback if noise/regression |

Optional artifact: `docs/PERF_9A_BASELINE.md` if tables would clutter [PERFORMANCE_BASELINE.md](PERFORMANCE_BASELINE.md).

### Out of scope (9A)

- Implementing optimisations in engine/book (→ **9B**, conditional).
- Persistence / command journal, TCP gateway, market data publisher.
- `std::list` / `std::map` wholesale replacement without profiler proof (6G lesson).
- CI benchmark regression gates.
- Committing `profiling/` trace bundles.
- UI, matching-semantics, or protocol layout changes.

### Constraints

- All benchmark claims must be **dated** and **machine-labelled**.
- `./scripts/verify.sh` must stay **164/164** after any doc-only edits.
- Prefer repeated runs (median/typical) over single-shot numbers.

### Acceptance criteria

- [ ] Dated Release baseline table in docs.
- [ ] Profiler section for post-8B `HEAD`.
- [ ] Hotspot list with engine vs non-engine split.
- [ ] Written 9B recommendation(s) tied to profiler evidence — or explicit “no change warranted.”
- [ ] No matching-semantics changes in 9A.
- [ ] `./scripts/verify.sh` passes.
- [ ] Human review + `/review-milestone` before `/advance-milestone` to **9B** or next queue item.

### Verification

```bash
./scripts/verify.sh
```

Benchmark/profiling commands documented in slice work — not gating CI unless you add doc-only instructions.

### References

- [MILESTONE_8C_DECISION.md](MILESTONE_8C_DECISION.md) §4–5
- [BENCHMARKING.md](BENCHMARKING.md) · [PERFORMANCE_BASELINE.md](PERFORMANCE_BASELINE.md) · [PERFORMANCE_ROADMAP.md](PERFORMANCE_ROADMAP.md) · [PROFILING_REPORT.md](PROFILING_REPORT.md)

### Previous milestone (8C — done)

| Deliverable | Commit |
|-------------|--------|
| Performance-first roadmap decision | `e85786b` |
