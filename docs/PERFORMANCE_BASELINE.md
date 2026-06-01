# Performance Baseline

This document records the Release-mode benchmark workflow used before performance optimisation work. Results here are local machine measurements and will vary by CPU, OS, compiler, build type, thermal state, and background load.

Clone-to-demo (no benchmark claims required): [DEMO_GUIDE.md](DEMO_GUIDE.md). Methodology: [BENCHMARKING.md](BENCHMARKING.md).

For profiler-backed **future optimisation candidates** (not yet implemented), see [PERFORMANCE_ROADMAP.md](PERFORMANCE_ROADMAP.md).

## Purpose

- Establish a repeatable baseline before changing performance-sensitive code.
- Keep binary protocol ingest measurements separate from engine matching measurements.
- Avoid using Debug-build timings in README, resume, or portfolio claims.
- Make before/after optimisation comparisons reproducible with the same command count, seed, build type, and machine.

## Release Mode Requirement

Use Release builds for benchmark numbers:

```bash
./scripts/benchmark_release.sh 100000 42
```

The script configures `build-release/` with:

```bash
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release
```

Then it builds and runs:

```bash
./build-release/binary_protocol_benchmark 100000 42
./build-release/matching_engine_benchmark 100000 42
```

Debug builds remain useful for correctness and debugging, but they are not representative for throughput or latency claims.

## Metrics

`binary_protocol_benchmark` reports phase-level wall-clock timings:

- Command generation time
- Binary write time and average ns/command
- Buffered binary read/decode time and average ns/command
- Buffered engine apply time and average ns/command
- Streaming read/decode/apply time and average ns/command
- Total measured phase time

`matching_engine_benchmark` reports:

- Runtime and throughput for `MatchingEngine::process`
- Per-command latency average/min/p50/p95/p99/max
- Trade count and final book sanity counters

## Current Local Baseline

These are local machine results from `./scripts/benchmark_release.sh 100000 42`. Treat them as a baseline for this machine only, not as portable performance claims.

| Benchmark | Commands | Seed | Build Type | Throughput | ns/command | Notes |
|-----------|----------|------|------------|------------|------------|-------|
| `binary_protocol_benchmark` binary write | 100,000 | 42 | Release | 10,850,056 commands/sec | 92 ns/command | OBK1 encode + file write phase |
| `binary_protocol_benchmark` buffered read/decode | 100,000 | 42 | Release | 18,978,630 commands/sec | 52 ns/command | OBK1 file read + decode into vector |
| `binary_protocol_benchmark` buffered engine apply | 100,000 | 42 | Release | 7,009,755 commands/sec | 142 ns/command | Decoded vector through `MatchingEngine` |
| `binary_protocol_benchmark` streaming read/decode/apply | 100,000 | 42 | Release | 5,677,772 commands/sec | 176 ns/command | One message decoded and applied at a time; 1,922,376 ns faster than buffered read/decode + apply on this run |
| `matching_engine_benchmark` engine loop | 100,000 | 42 | Release | 5,899,342 commands/sec | avg 140 ns | Direct synthetic command loop; p50 125 ns, p95 333 ns, p99 500 ns |

## Raw Local Output

```text
== Binary protocol benchmark (100000 commands, seed 42) ==
Benchmark: Binary protocol replay
Commands: 100000
Seed: 42
Binary file size: 6400000 bytes
Trades: 56086
Active orders: 10070
Resting quantity: 5039585
Phases:
  command generation: 0.0492102s
  binary write: 0.00921654s (10850056 commands/sec, 92 ns/command)
  buffered read/decode: 0.00526908s (18978630 commands/sec, 52 ns/command)
  buffered engine apply: 0.0142658s (7009755 commands/sec, 142 ns/command)
  buffered read/decode + apply total: 0.0195349s (19534917 ns)
  streaming read/decode/apply: 0.0176125s (5677772 commands/sec, 176 ns/command)
  buffered total minus streaming total: 1922376 ns
  total measured phases: 0.0955742s (95574166 ns)

== Matching engine benchmark (100000 commands, seed 42) ==
Benchmark: MatchingEngine synthetic workload
Commands: 100000
Seed: 42
Trades: 56086
Runtime: 0.016951s
Throughput: 5899342 commands/sec
Active orders: 10070
Resting quantity: 5039585
Latency ns:
  avg: 140
  min: 0
  p50: 125
  p95: 333
  p99: 500
  max: 43917
```

## Comparison Rules

For before/after optimisation comparisons:

1. Use the same machine, compiler, build type, command count, and seed.
2. Run `./scripts/verify.sh` before collecting benchmark numbers.
3. Run the Release benchmark script more than once if investigating noise.
4. Compare phase-appropriate metrics only: binary read/decode against binary read/decode, engine apply against engine loop.
5. Do not cite single-run local numbers as production exchange latency.

## 6C Allocation and Copy Reduction Pass

These results compare the 6B streaming baseline against the 6C allocation/copy reduction pass using:

```bash
./scripts/benchmark_release.sh 100000 42
```

Changes made in 6C:

- `MatchingEngine` now appends trade events directly into the output vector instead of creating a temporary `match_events` vector and copying/inserting it.
- `execute_new_order` reserves a small event-vector capacity for common accepted/trade/book-update paths.
- Benchmark workload generation moves completed `OrderCommand` objects into the command vector where ownership is clear.
- Binary benchmark timestamp attachment moves generated commands into `DecodedOrderCommand` values before writing.
- CSV split helpers reserve their expected column counts.
- Buffered binary reader uses `emplace_back` for decoded messages.

These are local single-run measurements and will vary by machine and system load.

**Benchmark stability (6D):** The before and after 6C numbers above each came from a **single** local Release run (`./scripts/benchmark_release.sh 100000 42`). Before claiming an improvement publicly or on a resume, run repeated benchmarks and report **typical** (median or central) results — see `./scripts/benchmark_repeat.sh` and [PROFILING_REPORT.md](PROFILING_REPORT.md).

| Metric | Before 6C | After 6C | Direction |
|--------|-----------|----------|-----------|
| Binary write | 10,850,056 commands/sec, 92 ns/command | 15,911,848 commands/sec, 62 ns/command | Improved on this run |
| Buffered read/decode | 18,978,630 commands/sec, 52 ns/command | 19,511,875 commands/sec, 51 ns/command | Roughly same/slightly improved |
| Buffered engine apply | 7,009,755 commands/sec, 142 ns/command | 8,688,726 commands/sec, 115 ns/command | Improved on this run |
| Streaming read/decode/apply | 5,677,772 commands/sec, 176 ns/command | 6,812,666 commands/sec, 146 ns/command | Improved on this run |
| Matching engine benchmark throughput | 5,899,342 commands/sec, avg 140 ns | 6,339,663 commands/sec, avg 125 ns | Improved on this run |

Raw after output:

```text
== Binary protocol benchmark (100000 commands, seed 42) ==
Benchmark: Binary protocol replay
Commands: 100000
Seed: 42
Binary file size: 6400000 bytes
Buffered trades: 56086
Streaming trades: 56086
Buffered active orders: 10070
Streaming active orders: 10070
Buffered resting quantity: 5039585
Streaming resting quantity: 5039585
Phases:
  command generation: 0.0412437s
  binary write: 0.00628463s (15911848 commands/sec, 62 ns/command)
  buffered read/decode: 0.00512508s (19511875 commands/sec, 51 ns/command)
  buffered engine apply: 0.0115092s (8688726 commands/sec, 115 ns/command)
  buffered read/decode + apply total: 0.0166342s (16634250 ns)
  streaming read/decode/apply: 0.0146785s (6812666 commands/sec, 146 ns/command)
  buffered total minus streaming total: 1955709 ns
  total measured phases: 0.0788411s (78841082 ns)

== Matching engine benchmark (100000 commands, seed 42) ==
Benchmark: MatchingEngine synthetic workload
Commands: 100000
Seed: 42
Trades: 56086
Runtime: 0.0157737s
Throughput: 6339663 commands/sec
Active orders: 10070
Resting quantity: 5039585
Latency ns:
  avg: 125
  min: 0
  p50: 83
  p95: 292
  p99: 500
  max: 72834
```

## 6E Targeted Hot-Path Optimisation

**Local, machine-dependent numbers only.** Do not treat these as portable throughput claims.

**Profiler:** macOS `sample` on `matching_engine_benchmark` with `--profile-engine-only` (benchmark harness only; production paths unchanged). The first full-run capture was generation-dominated; engine-only mode profiles the apply loop after workload generation.

**Code change:** `MatchingEngine::process_cancel` calls `OrderBook::cancel_order` once instead of `contains_order` then `cancel_order`. Unknown cancels still reject with `"unknown order id"`; semantics unchanged.

**Release benchmark comparison** (`./scripts/benchmark_repeat.sh 5 100000 42`, seed 42, Release):

| When | Matching engine throughput (5 runs) | Notes |
|------|--------------------------------------|--------|
| Before 6E cancel change (same branch session) | ~6.29–6.72M commands/sec; median ~6.38M | Overlaps post-6C single-run ~6.34M |
| After 6E cancel change (finalise session) | ~5.11–6.65M commands/sec; median ~6.46M | One outlier ~5.11M; spread typical of laptop noise |

**Conclusion:** Repeated before/after runs were **noisy** and did **not** show a clear throughput improvement attributable to the cancel-path change. The optimisation was justified by engine-only profiling (duplicate cancel lookup), not by a proven benchmark uplift. Further hash-table, allocation, and order-book structure work stays in milestones 6F/6G.

## 6F Caller-Owned Event Buffer Reuse

**Local, machine-dependent numbers only.**

**Code change:** `MatchingEngine::process_into` reuses a loop-scoped `std::vector<EngineEvent>` in matching-engine and binary-protocol engine apply benchmarks and CLI engine loops. `process()` unchanged for tests and compatibility (still allocates per call).

**Release benchmark comparison** (`./scripts/benchmark_repeat.sh 5 100000 42`, seed 42, Release):

| Phase | Before 6F (post-6E baseline, 5 runs) | After 6F (5 runs, same script) | Notes |
|-------|----------------------------------------|--------------------------------|--------|
| Matching engine synthetic | ~5.11–6.65M commands/sec; **median ~6.46M** | ~8.03–9.76M commands/sec; **median ~9.25M** | Hot loop uses `process_into`; one run ~8.03M |
| Buffered engine apply | *(6E session not re-run here)* | ~5.30–11.04M commands/sec; **median ~9.74M** | One outlier ~5.30M on run 3 |
| Streaming engine apply | *(6E session not re-run here)* | ~5.22–7.76M commands/sec; **median ~7.51M** | Same trade/invariant counts all runs |

**Conclusion:** On this machine/session, matching-engine median throughput rose versus the documented post-6E baseline (~6.46M → ~9.25M), consistent with removing per-command output-vector allocation in the benchmark hot loop. Runs were still noisy (spread ~8.0–9.8M on matching engine). Treat as **directional local evidence**, not a portable guarantee — no before/after binary-protocol phases were captured on the identical pre-6F commit in the same session. Correctness unchanged (56086 trades, 10070 active orders, 5039585 resting quantity on every run).

## 6G Order Lookup Reservation (slice 1)

**Local, machine-dependent numbers only.**

**Code change:** `OrderBook::reserve_active_orders` pre-sizes `order_lookup_`; matching-engine and binary-protocol benchmarks call `reserve_book_capacity(command_count / 10)` before the apply loop (seed-42 peak active orders ≈ 10% of command count). No change to `std::list` / `std::map` or matching rules.

**Profiler (pre-change):** Engine-only `sample` showed `add_order_to_side` with hash emplace/rehash and `operator new` (see `docs/PROFILING_REPORT.md` §6G).

**Release benchmark comparison** (`./scripts/benchmark_repeat.sh 5 100000 42`, seed 42, Release):

| Phase | Post-6F reference (5 runs, prior session) | After 6G slice 1 (5 runs, this session) | Notes |
|-------|------------------------------------------|----------------------------------------|--------|
| Matching engine synthetic | **median ~9.25M** cmd/s (~8.0–9.8M) | ~8.19–10.44M cmd/s; **median ~10.22M** | Ranges overlap; not a clear win |
| Buffered engine apply | **median ~9.74M** (prior session) | ~11.34–12.05M cmd/s; **median ~11.83M** | Noisy; same trade/invariant counts |

**Conclusion:** Reserve is profiler-justified for reducing hash growth work during ramp-up; repeated benchmarks on this machine **do not** show a stable throughput improvement versus the post-6F median. Treat any delta as noise unless reproduced on the same commit in one session. Correctness unchanged (56086 trades, 10070 active orders, 5039585 resting quantity on every run).

## 6H SPSC pipeline benchmark (slice 4)

**Local, machine-dependent numbers only. One Release run — not repeated here.**

**Goal of 6H:** correctness and equivalence (direct `process_into` vs `SpscCommandPipeline`), not throughput wins.

**Command:** `./build-release/ring_buffer_pipeline_benchmark 100000 42` (queue capacity defaults to command count).

| Path | Throughput (one run) | Notes |
|------|----------------------|--------|
| Direct `process_into` | ~8.36M commands/sec | Baseline hot loop |
| SPSC pipeline (`run_sequence`) | ~6.30M commands/sec | Enqueue-all-then-drain; **slower** on this run |

Event sequences and final book metrics (trades, active orders, best bid/ask, resting quantity) **matched** between paths on that run. The pipeline path adds queue overhead; **do not** claim the SPSC wrapper improves throughput from this single measurement. See [BENCHMARKING.md](BENCHMARKING.md) for how to re-run locally.
