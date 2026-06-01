# Active Milestone

**Source of truth** for `/run-active-milestone` and `/review-milestone`.

**Project root:** `cpp-low-latency-orderbook/`  
**Queue position:** See [MILESTONE_QUEUE.md](MILESTONE_QUEUE.md) — **6G is CURRENT**

---

## Milestone 6G — Order Book Data-Structure Optimisation

| Field | Value |
|-------|--------|
| **Status** | READY (workflow) |
| **Parent** | Milestone 6 — Performance and optimisation groundwork |
| **Test baseline** | 127 tests after 6F event-buffer reuse pass |

### Model recommendation

Use a **strong performance-focused coding model**. Book changes affect FIFO, iterator stability, and invariants; preserve behaviour exactly. Profile first; one narrow change per pass.

---

### Goal

Reduce **measured** cost of storing and looking up resting orders in `OrderBook` (e.g. `add_order_to_side`, `order_lookup_` insert/rehash, price-level work) **without** changing matching semantics, price-time priority, or book invariants.

Measure first (reuse 6E engine-only profiling or fresh Instruments/`sample`/perf), implement only what profiling confirms, and document before/after honestly as **local** and **machine-dependent**.

### First slice (planned — do not skip profiling)

1. **Re-profile** the engine apply loop after 6F (`process_into`) to confirm book-side hotspots still dominate.
2. If evidence points at `order_lookup_` rehash/emplace, implement **one** narrow optimisation such as `OrderBook::reserve_active_orders(size_t)` (or equivalent) calling `order_lookup_.reserve(...)` with a documented margin — only where call sites can supply a sensible expected active-order count or a profiled peak.
3. **Do not** in this first slice:
   - replace `std::list<Order>` at price levels
   - replace `std::map` bid/ask books
   - redesign the overall book model, add global allocators, or swap container families without dedicated tests and benchmarks

Later slices (only after profiling and a successful first slice) may consider other items in [PERFORMANCE_ROADMAP.md](PERFORMANCE_ROADMAP.md); they are **not** part of the initial 6G implementation pass unless explicitly approved.

### Scope

- Re-profile to confirm top book-side costs (hash rehash on `order_lookup_`, `std::map` price-level operations, `std::list` node churn, redundant lookups)
- Implement **one focused** improvement for a confirmed hot site (first slice: likely `order_lookup_` reserve only)
- Preserve **all** book invariants: FIFO within a price level, iterator stability on `reduce`, price-time priority, `validate_invariants()` semantics
- Keep **replay path** (`MarketEvent` → `OrderBook`) and **engine path** (`OrderCommand` → `MatchingEngine`) behaviourally identical
- Add tests if reserve or any API addition could affect observable behaviour
- Update `docs/PERFORMANCE_BASELINE.md` with before/after repeated Release runs
- Short notes in `docs/PROFILING_REPORT.md` on what changed and why

### Out of scope

- Matching semantics, priority rules, or user-visible behaviour changes
- Memory pool / event-buffer reuse (that was **6F**)
- SPSC queue, multithreading, or lock-free pipeline work (that is **6H**)
- Replacing `std::list`, `std::map`, or the book storage model in the **first** 6G slice
- New networking, persistence, replication, or exchange connectivity
- Binary protocol semantic changes
- Benchmark “gaming” (removing validation or changing measurement semantics)
- Large third-party container dependencies without justification

### Acceptance criteria

- [ ] `./scripts/verify.sh` passes (add tests if new book API or behaviour edge cases)
- [ ] Change is **profiler-justified** (tool used, book site targeted, documented)
- [ ] Book invariants preserved (`validate_invariants()` and FIFO/price-time tests still pass)
- [ ] Release benchmark comparison with repeated runs:
  - same machine, build type, command count, seed
  - compare **typical/median** results, not a single outlier
- [ ] `docs/PERFORMANCE_BASELINE.md` updated with before/after and clear “local numbers” caveat
- [ ] No later-milestone work (6H/7A+) included in the same change set

### Required verification

From project root:

```bash
./scripts/verify.sh
```

Then (Release, repeated):

```bash
./scripts/benchmark_repeat.sh 5 100000 42
```

Optional: engine-only profiling if measuring apply-loop / book costs:

```bash
./build-release/matching_engine_benchmark 1000000 42 --profile-engine-only
```

### Key docs

- [PERFORMANCE_ROADMAP.md](PERFORMANCE_ROADMAP.md)
- [PROFILING_REPORT.md](PROFILING_REPORT.md)
- [BENCHMARKING.md](BENCHMARKING.md)
- [PERFORMANCE_BASELINE.md](PERFORMANCE_BASELINE.md)
- [DEVELOPMENT_RULES.md](DEVELOPMENT_RULES.md)
- [ARCHITECTURE.md](ARCHITECTURE.md)

### Human review before advance

After `/run-active-milestone` completes, run `/review-milestone`, then human approval before `/advance-milestone`.
