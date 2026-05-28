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

## Next optimisation candidates

Ordered roughly by risk and dependency (design before pools, measure before rewriting the book):

1. **Targeted MatchingEngine hot-path profiling** — confirm match loop and event emission cost with Instruments or perf
2. **Trade/event buffer reuse** — reduce per-command vector allocations if profiling shows them
3. **Order storage allocation reduction** — reserve maps, reduce rehashing, avoid redundant lookups (without changing semantics)
4. **Symbol representation improvements** — fixed buffers or interned symbols if string work shows up hot
5. **Memory pool / object pool** — only after hot allocations are identified; higher complexity
6. **Optional SPSC queue later** — pipeline design for a future threaded ingest path; not required for current single-threaded CLI/benchmarks

Each item should be benchmark-driven: one change, `./scripts/verify.sh`, repeated Release runs, update [PERFORMANCE_BASELINE.md](PERFORMANCE_BASELINE.md) with typical results.

## Related documentation

- [BENCHMARKING.md](BENCHMARKING.md) — harness details and repeated-run workflow
- [PERFORMANCE_BASELINE.md](PERFORMANCE_BASELINE.md) — recorded local baselines and 6C comparison
- [ARCHITECTURE.md](ARCHITECTURE.md) — OrderBook vs MatchingEngine boundaries
