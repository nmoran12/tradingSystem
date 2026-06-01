# Profiling Report

Guidance for measuring and interpreting performance in this project before further optimisation work. This document does not replace running benchmarks on your machine; it explains *how* to profile and *where* to look next.

## Purpose of profiling

Profiling answers **where time and memory go** in a run, not just how fast a synthetic benchmark finished. Use it to:

- Confirm suspected hot paths before rewriting data structures
- Separate engine matching cost from binary I/O and decode cost
- Avoid optimising cold paths or one-off allocations
- Validate that a change improved the intended phase without regressing others

The project’s benchmarks (`binary_protocol_benchmark`, `matching_engine_benchmark`) give repeatable **phase-level** throughput. Profilers add **call-stack detail** inside those phases.

## Current benchmark baseline summary

Release-mode baselines are recorded in [PERFORMANCE_BASELINE.md](PERFORMANCE_BASELINE.md). Typical workflow:

```bash
./scripts/benchmark_release.sh 100000 42
```

At 100k commands / seed 42 on a local Release build, recent work shows roughly:

| Area | Order of magnitude (local, not portable) |
|------|------------------------------------------|
| Binary write | ~10–16M commands/sec |
| Buffered read/decode | ~19–20M commands/sec |
| Buffered engine apply | ~7–9M commands/sec |
| Streaming read/decode/apply | ~6–7M commands/sec |
| Matching engine synthetic loop | ~6M commands/sec, p50 ~80–125 ns |

Trade counts and book state should match across buffered vs streaming paths for the same workload. See the baseline doc for exact single-run numbers and 6C before/after tables.

## Why single benchmark runs are noisy

Wall-clock benchmarks on a laptop or shared machine are affected by:

- CPU frequency scaling and thermal throttling
- Background processes (browser, indexing, OS updates)
- First-run effects (cold disk cache, JIT not applicable here but CMake rebuild cost matters)
- File system cache when re-reading the same temporary `.obk` file
- Compiler and linker differences between Debug and Release

A single Release run can look **10–30%+ better or worse** than the next run without any code change. The 6C improvement table in [PERFORMANCE_BASELINE.md](PERFORMANCE_BASELINE.md) came from **one** before run and **one** after run — useful as a directional signal, not as a guaranteed production claim.

## Recommended benchmark method

1. **Release build only** — use `./scripts/benchmark_release.sh` (configures `build-release/` with `-DCMAKE_BUILD_TYPE=Release`).
2. **Same machine** — do not compare numbers across different CPUs or OS versions without noting it.
3. **Same command count and seed** — default `100000` and `42` match documented baselines.
4. **Multiple runs** — use repeated benchmarks:

   ```bash
   ./scripts/benchmark_repeat.sh 5 100000 42
   ```

   Or, to save timestamped logs and a lightweight summary:

   ```bash
   ./scripts/repeated-benchmark.sh 5 100000 42
   ```

5. **Compare typical results** — eyeball median or central tendency for each phase; ignore one outlier fast run.
6. **Run verify first** — `./scripts/verify.sh` ensures correctness before trusting performance numbers.
7. **Do not mix Debug and Release** — Debug is for debugging; Release is for throughput claims.

See [BENCHMARKING.md](BENCHMARKING.md) for metric definitions and output format.

## Current suspected hot spots

These areas are the most likely targets for the next profiling sessions (based on architecture and benchmark phase split):

| Area | Why investigate |
|------|-----------------|
| **MatchingEngine apply path** | `process` and match helpers dominate buffered/streaming engine apply and the matching engine benchmark |
| **Order storage / order lookup** | `OrderBook` maps and price-level structure on every add/cancel/match |
| **Trade/event output vector construction** | Per-command `EngineEvent` vector growth and append patterns (partially improved in 6C) |
| **String / symbol handling** | `std::string` on commands, CSV/binary decode, symbol padding |
| **Binary read/decode path** | 64-byte message read, decode, and callback overhead in streaming vs buffered replay |

Profiling should confirm which of these dominates **your** workload before large refactors.

## Tools

### Linux / WSL

| Tool | Use |
|------|-----|
| **perf** (`perf record` / `perf report`) | CPU flame graphs and hot symbols |
| **heaptrack** | Heap allocations and temporary peaks |
| **valgrind --tool=callgrind** + **kcachegrind** | Call-graph profiling (slow, precise) |

Example:

```bash
perf record -g ./build-release/matching_engine_benchmark 100000 42
perf report
```

### macOS

| Tool | Use |
|------|-----|
| **Instruments** (Time Profiler, Allocations) | GUI profiling of CPU and memory |
| **time** | Quick wall-clock sanity check |
| **sample** | Lightweight stack samples on a running PID |
| **Activity Monitor** | Spot thermal throttling and competing processes |

Build Release targets first, then profile the same binaries under `build-release/` that `benchmark_release.sh` uses.

## 6E profiling summary

| Step | Result |
|------|--------|
| Initial `sample` on normal benchmark | Dominated by `WorkloadGenerator::generate()` — not engine/book |
| Engine-only mode added | `--profile-engine-only` generates commands, pauses, then runs the apply loop only |
| Engine-only `sample` | `MatchingEngine` / `OrderBook` frames visible (`contains_order`, hash insert/rehash, `add_order_to_side`, `execute_order`, allocations) |
| Single optimisation | `process_cancel`: one `cancel_order()` instead of `contains_order` + `cancel_order` |
| Post-change engine-only `sample` | Harness still captures engine loop; cancel stacks show `cancel_order` (pre-change cancel path showed `contains_order`); `contains_order` remains hot on new-order paths — unchanged in 6E |
| Benchmark repeat after change | Noisy; no clear throughput win — see [PERFORMANCE_BASELINE.md](PERFORMANCE_BASELINE.md) §6E |

Engine-only profiling (benchmark harness only):

```bash
./build-release/matching_engine_benchmark 1000000 42 --profile-engine-only
# In another terminal during countdown:
sample <pid> 10 -file profiling/6e/matching_engine_engine_only_sample.txt
```

Does not change production behaviour, matching/book/protocol semantics, or the normal benchmark path.

**Deferred to later milestones (at time of 6E):** order-book data-structure changes (6G), queues and threading (6H+).

## 6F event output buffer reuse

| Decision | Detail |
|----------|--------|
| **Profiler target** | Engine-only `sample` showed `operator new` under `execute_new_order` and `std::vector<EngineEvent>::__init_with_size` on cancel returns — per-command output vector heap churn |
| **Rejected approach** | Engine-owned `event_buffer_` with `return std::move(buffer)` — move-return transfers the heap block to the caller; when callers discard the vector each iteration (benchmark/CLI), the engine must re-allocate and can do **more** work than today |
| **Implemented approach** | `MatchingEngine::process_into(command, events)` — `events.clear()` at entry (retains capacity); hot loops reuse one caller-owned `std::vector<EngineEvent>` |
| **Compatibility** | `process()` remains; allocates a local vector and delegates to `process_into` |
| **Hot paths updated** | `matching_engine_benchmark`, `binary_protocol_benchmark` engine apply phases, `main.cpp` engine loops |

`process()` callers still pay per-call vector allocation; reuse benefit requires adopting `process_into` at the call site.

## 6G order lookup reservation (slice 1)

| Step | Result |
|------|--------|
| Pre-change engine-only `sample` | `profiling/6g/engine_only_sample.txt` (local, not in git) — `add_order_to_side` with `__hash_table` emplace / `__do_rehash` and `operator new` under resting adds |
| Change | `OrderBook::reserve_active_orders(n)` → `order_lookup_.reserve(n)`; `MatchingEngine::reserve_book_capacity(n)`; benchmarks call `command_count / 10` from seed-42 peak active ratio |
| Containers unchanged | `std::list`, `std::map`, `OrderLocation`, matching semantics unchanged |
| Benchmark repeat | No clear median throughput win vs post-6F session; see [PERFORMANCE_BASELINE.md](PERFORMANCE_BASELINE.md) §6G |

## 6G post-reserve engine-only profile (slice 1 review)

Captured locally: `profiling/6g/post_reserve_engine_only_sample.txt` (not in git). Harness: `./build-release/matching_engine_benchmark 1000000 42 --profile-engine-only` with `reserve_book_capacity(command_count / 10)` already enabled.

```bash
sample <pid> 10 -file profiling/6g/post_reserve_engine_only_sample.txt
```

| Signal | Pre-reserve sample (no lookup reserve) | Post-reserve sample |
|--------|----------------------------------------|---------------------|
| `add_order_to_side` (collapsed ≥5) | 5 | **13** |
| `remove_order_at_location` (collapsed ≥5) | 10 | **11** |
| `__do_rehash` on `order_lookup_` | Multiple stacks under emplace | **~1** stack (residual growth edge cases) |
| `emplace` + `operator new` on lookup insert | Common | Still present (6 emplace samples) |
| `std::__tree_balance_after_insert` / `__tree_remove` under add/remove | Present | Present on add/remove paths |
| `operator new` at list `push_back` offsets | Present | Present (5+ samples on add path) |

**Interpretation:** Lookup **rehash** pressure is much lower after `reserve_active_orders`, consistent with slice 1 intent. Remaining cost on the apply loop is **mixed**: resting adds still allocate (list nodes, map price-level nodes, occasional hash bucket growth) and match/cancel paths spend time in `remove_order_at_location` (list erase + map level cleanup + hash erase).

**Recommendation:** Do **not** start a `std::list` or `std::map` container swap from this sample alone. Treat 6G slice 1 as complete; use heap allocation attribution (Instruments Allocations / heaptrack) before any slice 2, or stop 6G book structure work and advance the queue when human review agrees.

## Future optimisation work

Profiler-backed candidates, risk notes, and the required milestone process are documented in **[PERFORMANCE_ROADMAP.md](PERFORMANCE_ROADMAP.md)** (section *Future Performance Optimisation Candidates*).

**Recently completed:** order lookup pre-reserve (**6G slice 1**). **Next profiling targets:** list/map node allocation if book path remains hot — see roadmap; no container swap in slice 1.

## Related documentation

- [PERFORMANCE_ROADMAP.md](PERFORMANCE_ROADMAP.md) — future candidates and optimisation process
- [BENCHMARKING.md](BENCHMARKING.md) — harness details and repeated-run workflow
- [PERFORMANCE_BASELINE.md](PERFORMANCE_BASELINE.md) — recorded local baselines and 6C comparison
- [ARCHITECTURE.md](ARCHITECTURE.md) — OrderBook vs MatchingEngine boundaries
