# Active Milestone

**Source of truth** for `/run-active-milestone` and `/review-milestone`.

**Project root:** `cpp-low-latency-orderbook/`  
**Queue:** [MILESTONE_QUEUE.md](MILESTONE_QUEUE.md) — row **20**

---

## CURRENT: 9B — One Measured Hot-Path Optimisation (Conditional)

**Status:** **Slice 1 complete — slice 2 not implemented** (allocation evidence mixed; list node pool **not** justified).

### Goal

If and only if measurement confirms a justified target, implement **at most one** narrow optimisation. **Slice 1 is done; slice 2 was skipped** per decision rule.

### Slice 1 result (2026-06-01)

See [PROFILING_REPORT.md](PROFILING_REPORT.md) §9B slice 1.

| Finding | Detail |
|---------|--------|
| Tool | `sample` during apply (`2000000` cmds, seed 42, engine-only) |
| Dominant add-path allocs | **Mixed** — hash `order_lookup_` emplace **~51–58** samples, list `push_back` **~46**, map **~29–36** |
| Event vector | Weak signal (~11 `process_into` collapsed samples) |
| Slice 2 | **Stopped** — does not meet “list clearly dominates” threshold |

### Acceptance criteria

- [x] Slice 1 alloc profile completed and documented.
- [x] Explicit stop: “no implementation warranted” for list-node pool.
- [ ] If slice 2 had run: 164/164 tests, benchmarks — **N/A (not run)**.
- [x] No matching-semantics changes.
- [ ] Human review + `/review-milestone` before `/advance-milestone`.

### Verification

```bash
./scripts/verify.sh
```

### References

- [MILESTONE_9A_PERFORMANCE_PLAN.md](MILESTONE_9A_PERFORMANCE_PLAN.md) §10
- [PROFILING_REPORT.md](PROFILING_REPORT.md) §9B slice 1

### Previous milestone (9A — done)

| Deliverable | Commit |
|-------------|--------|
| Performance baseline and optimisation plan | `191ddab` |
