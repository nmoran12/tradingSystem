# Active Milestone

**Source of truth** for `/run-active-milestone` and `/review-milestone`.

**Project root:** `cpp-low-latency-orderbook/`  
**Queue position:** See [MILESTONE_QUEUE.md](MILESTONE_QUEUE.md) — **6F is CURRENT**

---

## Milestone 6F — Memory Pool / Object Pool

| Field | Value |
|-------|--------|
| **Status** | READY (workflow) |
| **Parent** | Milestone 6 — Performance and optimisation groundwork |
| **Test baseline** | 123 tests after 6E targeted hot-path pass |

### Model recommendation

Use a **strong performance-focused coding model**. Pooling touches allocation patterns and must preserve behaviour. Profile before adding pools; keep scope minimal.

---

### Goal

Reduce **identified hot allocations** in the engine/book path using **scoped, optional object pooling** (or equivalent reuse), without changing matching semantics or user-visible behaviour.

Measure first (reuse 6E engine-only profiling or fresh Instruments/`sample`/perf), pool only what profiling shows, and document results honestly.

### Scope

- Re-profile or extend 6E findings to confirm top allocation sites (e.g. `EngineEvent` vectors, `Order` construction, hash/map growth if pooling is not the right tool)
- Implement **one focused pool or reuse strategy** for a confirmed hot type, such as:
  - small fixed-size object pool for frequently allocated structs
  - thread-local or engine-owned buffer reuse for per-command `std::vector<EngineEvent>` if profiling justifies it
- Keep pools **local and explicit** (no global hidden allocator for the whole project)
- Add tests for pool correctness (acquire/release, exhaustion policy, no double-free)
- Update `docs/PERFORMANCE_BASELINE.md` with before/after repeated Release runs, labelled **local** and **machine-dependent**
- Short notes in `docs/PROFILING_REPORT.md` on what was pooled and why

### Out of scope

- Behaviour changes to `MatchingEngine` or `OrderBook` (matching semantics, priority rules, book invariants)
- Order-book data-structure rewrites (that is **6G**)
- SPSC queue, multithreading, or lock-free pipeline work (that is **6H**)
- New networking, persistence, replication, or exchange connectivity
- Replacing `std::vector<EngineEvent>` return type with callbacks (future refactor)
- Benchmark “gaming” (removing validation or changing measurement semantics)
- Pooling everything preemptively without profiler evidence

### Acceptance criteria

- [ ] `./scripts/verify.sh` passes (add tests for new pool/reuse behaviour)
- [ ] Pooling is **profiler-justified** (tool used, allocation site targeted, documented)
- [ ] Release benchmark comparison with repeated runs:
  - same machine, build type, command count, seed
  - compare **typical/median** results, not a single outlier
- [ ] `docs/PERFORMANCE_BASELINE.md` updated with before/after and clear “local numbers” caveat
- [ ] No later-milestone work (6G/6H/7A+) included in the same change set

### Required verification

From project root:

```bash
./scripts/verify.sh
```

Then (Release, repeated):

```bash
./scripts/benchmark_repeat.sh 5 100000 42
```

Optional: engine-only profiling if measuring apply-loop allocations:

```bash
./build-release/matching_engine_benchmark 1000000 42 --profile-engine-only
```

### Key docs

- [PROFILING_REPORT.md](PROFILING_REPORT.md)
- [PERFORMANCE_ROADMAP.md](PERFORMANCE_ROADMAP.md)
- [BENCHMARKING.md](BENCHMARKING.md)
- [PERFORMANCE_BASELINE.md](PERFORMANCE_BASELINE.md)
- [DEVELOPMENT_RULES.md](DEVELOPMENT_RULES.md)
- [ARCHITECTURE.md](ARCHITECTURE.md)

### Human review before advance

After `/run-active-milestone` completes, run `/review-milestone`, then human approval before `/advance-milestone`.
