# Performance Baseline

This document records the Release-mode benchmark workflow used before performance optimisation work. Results here are local machine measurements and will vary by CPU, OS, compiler, build type, thermal state, and background load.

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
