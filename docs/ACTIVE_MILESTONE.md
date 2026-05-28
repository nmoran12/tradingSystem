# Active Milestone

**Source of truth** for `/run-active-milestone` and `/review-milestone`.

**Project root:** `cpp-low-latency-orderbook/`  
**Queue position:** See [MILESTONE_QUEUE.md](MILESTONE_QUEUE.md) — **6E is CURRENT**

---

## Milestone 6E — Targeted Hot-Path Optimisation

| Field | Value |
|-------|--------|
| **Status** | READY (workflow) |
| **Parent** | Milestone 6 — Performance and optimisation groundwork |
| **Test baseline** | 123 tests after 6D docs-only stability/profiling pass |

### Model recommendation

Use a **strong performance-focused coding model**. This milestone touches hot paths and must preserve behaviour.

---

### Goal

Make **small, profiler-guided** improvements in confirmed hot paths (engine apply / decode / book lookup / event emission) while preserving all correctness and user-visible behaviour.

This milestone is intentionally narrow: **measure first**, change only what profiling and benchmarks justify, and document results honestly.

### Scope

- Run profiler(s) and identify top hot symbols in:
  - `matching_engine_benchmark`
  - `binary_protocol_benchmark` (buffered vs streaming)
- Implement **1–3** small, low-risk changes in confirmed hot paths, such as:
  - reserve/pre-size where sizes are known
  - remove redundant lookups or copies on the hot path
  - reduce temporary allocations in event/trade construction
  - tighten symbol/string handling where profiling shows it matters
- Update `docs/PERFORMANCE_BASELINE.md` with before/after results, clearly labelled as **local** and **machine-dependent**
- If helpful, add short notes to `docs/PROFILING_REPORT.md` describing what was profiled and what changed

### Out of scope

- Behaviour changes to `MatchingEngine` or `OrderBook` (matching semantics, priority rules, book invariants)
- New networking, persistence, replication, or exchange connectivity
- Memory pools / custom allocators (that is 6F)
- SPSC queue / multithreading / lock-free pipeline work (later milestone)
- Data-structure rewrites of the order book (that is 6G)
- Benchmark “gaming” (removing validation or changing measurement semantics)

### Acceptance criteria

- [ ] `./scripts/verify.sh` passes (tests unchanged except any new non-timing regression tests)
- [ ] Any optimisations are **profiler-justified** (record the tool used and the top hot spot targeted)
- [ ] Release benchmark comparison is done with repeated runs:
  - same machine, build type, command count, seed
  - compare **typical/median** results, not a single outlier
- [ ] `docs/PERFORMANCE_BASELINE.md` updated with before/after and clear “local numbers” caveat
- [ ] No later-milestone work (6F/6G/6H/7A+) included

### Required verification

From project root:

```bash
./scripts/verify.sh
```

Then (Release, repeated):

```bash
./scripts/benchmark_repeat.sh 5 100000 42
```

### Key docs

- [PROFILING_REPORT.md](PROFILING_REPORT.md)
- [BENCHMARKING.md](BENCHMARKING.md)
- [PERFORMANCE_BASELINE.md](PERFORMANCE_BASELINE.md)
- [DEVELOPMENT_RULES.md](DEVELOPMENT_RULES.md)
- [ARCHITECTURE.md](ARCHITECTURE.md)

### Human review before advance

After `/run-active-milestone` completes, run `/review-milestone`, then human approval before `/advance-milestone`.
