# Active Milestone

**Source of truth** for `/run-active-milestone` and `/review-milestone`.

**Project root:** `cpp-low-latency-orderbook/`  
**Queue:** [MILESTONE_QUEUE.md](MILESTONE_QUEUE.md) — row **20**

**Model recommendation:** Default agent; careful with book storage and allocator lifetimes. Stronger model if slice 2 touches `OrderBook` iterators or pools.

---

## CURRENT: 9B — One Measured Hot-Path Optimisation (Conditional)

**Status:** `READY` — not started. Implement **only** if 9A evidence supports the target; **slice 1 (measurement) is required before any book code change.**

### Goal

If and only if measurement confirms a justified target, implement **at most one** narrow optimisation and prove **no correctness regression** with documented, repeated Release benchmarks. Do **not** claim throughput wins unless measured.

### Background (9A outcome)

[MILESTONE_9A_PERFORMANCE_PLAN.md](MILESTONE_9A_PERFORMANCE_PLAN.md) (commit `191ddab`) recommends a **two-slice** milestone:

| Slice | Work |
|-------|------|
| **1 (required)** | Byte-ranked allocation profile (Instruments Allocations or Linux `heaptrack`) on engine-only apply loop (1M–2M commands, seed 42, `reserve_book_capacity` enabled) |
| **2 (conditional)** | Reduce **`std::list<Order>` node heap traffic** in `OrderBook::add_order_to_side` (e.g. node pool/arena) **only if** slice 1 shows list nodes dominate apply-loop heap |

**Alternate if slice 1 inconclusive:** benchmark harness throughput mode (disable per-command `steady_clock`) — measurement-only, no production semantics change.

### Slices

| # | Slice | Deliverable |
|---|--------|-------------|
| 1 | Byte-ranked alloc profile | Dated table in [PROFILING_REPORT.md](PROFILING_REPORT.md); raw traces in `profiling/` only |
| 2a | List-node optimisation (if justified) | Narrow `OrderBook` change + tests if invariants at risk |
| 2b | Or benchmark throughput mode | Flag/target for stable cmd/s if 2a not justified |
| 3 | Verification | `./scripts/verify.sh`; `./scripts/benchmark_repeat.sh 5 100000 42`; update [PERFORMANCE_BASELINE.md](PERFORMANCE_BASELINE.md) |

### Out of scope (9B)

- `std::list` / `std::map` wholesale replacement without byte-ranked proof (6G/9A lesson).
- Persistence, TCP gateway, market data publisher.
- Multithreaded matching or SPSC throughput claims.
- OBK1 / CSV / replay format changes.
- CI benchmark regression gates.
- Committing `profiling/` trace bundles.
- Multiple unrelated optimisations in one milestone.

### Constraints

- Change must map to a **9A / 6G profiler line item** or slice 1 alloc table row.
- Preserve matching rules, FIFO, rejection strings, protocol behaviour.
- Run `./scripts/verify.sh` before and after; add tests only if behaviour surface could change.
- Report **medians** over repeated Release runs — not single-shot claims.

### Acceptance criteria

- [ ] Slice 1 alloc profile completed and documented (or explicit stop with “no implementation warranted”).
- [ ] If slice 2 runs: one narrow change; **164/164** tests; workload invariants + equivalence tests still pass.
- [ ] Before/after benchmark table (same seed/count, Release, repeated runs) in [PERFORMANCE_BASELINE.md](PERFORMANCE_BASELINE.md).
- [ ] Honest conclusion if delta is within noise (document no-merge / stop).
- [ ] No matching-semantics regression.
- [ ] Human review + `/review-milestone` before `/advance-milestone`.

### Verification

```bash
./scripts/verify.sh
```

Profiling and benchmarks per [MILESTONE_9A_PERFORMANCE_PLAN.md](MILESTONE_9A_PERFORMANCE_PLAN.md) §8 and [BENCHMARKING.md](BENCHMARKING.md).

### References

- [MILESTONE_9A_PERFORMANCE_PLAN.md](MILESTONE_9A_PERFORMANCE_PLAN.md) §7–9
- [MILESTONE_8C_DECISION.md](MILESTONE_8C_DECISION.md) §5
- [PROFILING_REPORT.md](PROFILING_REPORT.md) · [PERFORMANCE_ROADMAP.md](PERFORMANCE_ROADMAP.md)

### Previous milestone (9A — done)

| Deliverable | Commit |
|-------------|--------|
| Performance baseline and optimisation plan | `191ddab` |
