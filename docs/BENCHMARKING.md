# Benchmarking

Guide to the benchmark harnesses and how to interpret results.

## Purpose

The benchmarks turn `cpp-low-latency-orderbook` from a correctness-only project into a **performance-measured** systems project. They answer:

- How many `OrderCommand`s per second can `MatchingEngine::process` handle on this machine?
- What is the per-command latency distribution (avg, percentiles, max)?
- How much local overhead does the fixed-width OBK1 binary command path add for file write/read/decode?
- Does the book remain structurally valid after a long synthetic run?

This is **not** a production exchange benchmark. There is no networking, no lock-free queue, and no custom allocator in the hot path yet.

## What `matching_engine_benchmark` measures

**Scope:** time spent in `MatchingEngine::process(command)` for each command in a pre-generated workload, plus aggregate counters.

**Includes:**

- Command parsing is **not** included (commands built in memory by `WorkloadGenerator`).
- Matching, book updates, and event vector growth **are** included.
- Post-run `OrderBook::validate_invariants()` sanity check.

**Does not measure:**

- CSV parsing, CLI I/O, or `EngineEventPrinter`
- Multi-threaded contention (single-threaded loop)

## What `binary_protocol_benchmark` measures

**Scope:** deterministic command generation, OBK1 binary file write, buffered OBK1 file read/decode, buffered engine apply, and streaming read/decode/apply through `MatchingEngine`.

**Timing phases are reported separately:**

- Command generation time
- Binary write time using `protocol::write_order_commands_binary`
- Buffered binary read/decode time using `protocol::read_order_commands_binary`
- Buffered engine apply time for decoded `OrderCommand`s
- Streaming read/decode/apply time using `protocol::stream_order_commands_binary`
- Total measured phase time

The buffered path loads all decoded commands into a vector before applying them to the engine. The streaming path reads one 64-byte message, decodes it, immediately invokes a callback, and applies the command without retaining the whole decoded file. This reduces decoded-command memory pressure and gives a cleaner measurement for replay pipelines that do not need random access to all commands.

**Important:** the binary benchmark reports phase-level wall-clock totals and average nanoseconds per command for write/read/decode/apply phases. It does **not** report p50/p95/p99 for binary decode because it does not measure per-command decode latency. Do not claim streaming is faster unless the benchmark run shows it for the same command count, seed, build type, and machine.

**Does not measure:**

- CLI printing or `EngineEventPrinter`
- CSV parsing
- Networking or real exchange connectivity
- Multithreaded contention

## WorkloadGenerator

**Location:** `include/benchmarks/WorkloadGenerator.hpp`, `src/benchmarks/WorkloadGenerator.cpp`

**Behaviour:**

- Deterministic: same `WorkloadConfig` + `random_seed` → identical command sequence
- Symbols are assigned from a fixed configured set (default: `AAPL`, `MSFT`, `NVDA`)
- Default mix (percentages must sum to 1.0):
  - **70%** new limit orders
  - **10%** new market orders
  - **10%** cancels (target random active limit order; removes from active pool)
  - **10%** modifies (target random active limit order)
- Price: uniform around `price_midpoint` ± `price_half_range` (default midpoint 10000, half-range 500)
- Quantity: uniform in `[min_quantity, max_quantity]` (default 1–1000)
- Only **limit** orders are tracked for cancel/modify targeting (market orders do not enter active pool)

**Default benchmark config** (`benchmark_matching_engine.cpp`):

| Parameter | Default |
|-----------|---------|
| `command_count` | 1,000,000 |
| `random_seed` | 42 |
| Limit / market / cancel / modify | 70% / 10% / 10% / 10% |

**CLI overrides:**

```bash
./build/matching_engine_benchmark [command_count] [random_seed]
```

Example: `./build/matching_engine_benchmark 100000 42`

`binary_protocol_benchmark` uses the same workload generator, with a smaller default command count for quick local binary-path measurements:

| Parameter | Default |
|-----------|---------|
| `command_count` | 100,000 |
| `random_seed` | 42 |

```bash
./build/binary_protocol_benchmark [command_count] [random_seed]
```

Example: `./build/binary_protocol_benchmark 100000 42`

## Metrics reported

| Metric | Meaning |
|--------|---------|
| Commands | Total commands processed |
| Trades | Count of `EngineEventType::Trade` emitted |
| Runtime | Wall-clock seconds for full loop |
| Throughput | Commands / second |
| Active orders | `OrderBook::active_order_count()` at end |
| Resting quantity | `OrderBook::total_resting_quantity()` at end |
| Latency avg/min/max | Per-command `process()` duration (ns), via `LatencyTracker` |
| p50 / p95 / p99 | Nearest-rank percentiles of per-command latency |

Additional `binary_protocol_benchmark` metrics:

| Metric | Meaning |
|--------|---------|
| Seed | Workload RNG seed |
| Binary file size | Size of temporary `.obk` file; should be `commands * 64` bytes |
| Command generation | Wall-clock time to build the deterministic in-memory workload |
| Binary write | Wall-clock time to encode and write OBK1 messages |
| Buffered read/decode | Wall-clock time to read and decode the OBK1 file into a vector |
| Buffered engine apply | Wall-clock time to process the decoded vector through `MatchingEngine` |
| Streaming read/decode/apply | Wall-clock time to read, decode, and immediately apply each message |
| Buffered total minus streaming total | Positive means streaming was faster on that run; negative means buffered was faster |
| commands/sec | Phase throughput for write/read/decode/apply |
| ns/command | Phase average only; not a latency distribution |

## Sanity checks (after run)

`OrderBook::validate_invariants()` verifies:

- Order count in books matches `order_lookup_` size
- No crossed book when both bid and ask exist (`best_bid < best_ask`)
- All resting quantities > 0
- Order price matches its level key

Additional benchmark checks:

- `active_order_count <= command_count` (sanity bound)

Failure prints to stderr and exits non-zero.

## How to run

From project root `cpp-low-latency-orderbook/`:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/matching_engine_benchmark
./build/matching_engine_benchmark 1000000 42
./build/binary_protocol_benchmark
./build/binary_protocol_benchmark 100000 42
```

Debug builds work but are not representative for throughput comparisons.

## Release Baseline Workflow

For baseline numbers and before/after optimisation comparisons, prefer the Release benchmark script:

```bash
./scripts/benchmark_release.sh
./scripts/benchmark_release.sh 1000000 42
```

The script configures `build-release/` with `-DCMAKE_BUILD_TYPE=Release`, builds `binary_protocol_benchmark` and `matching_engine_benchmark`, then runs both with the same command count and seed. This keeps measurement setup repeatable and separates it from the normal debug-ish `build/` directory used by `./scripts/verify.sh`.

Do not use Debug build numbers in README, resume, or portfolio claims. Debug numbers are useful for catching regressions in benchmark executables, but not for performance statements.

For fair comparisons:

- Run on the same machine with the same command count and seed.
- Compare the same metric before and after a change.
- Treat binary read/decode throughput separately from engine apply throughput.
- Include machine/build context when documenting results.
- Avoid universal claims; results vary by CPU, OS, compiler, build flags, and system load.

See [PERFORMANCE_BASELINE.md](PERFORMANCE_BASELINE.md) for the current local baseline methodology and recorded results.

## Repeated benchmarks (stability)

Single Release runs can vary with CPU scaling, background load, and disk cache. For before/after optimisation work, run the same workload several times and compare **typical** results, not the best single run:

```bash
./scripts/benchmark_repeat.sh              # 5 runs, 100000 commands, seed 42
./scripts/benchmark_repeat.sh 5 100000 42  # explicit repetitions, count, seed
./scripts/benchmark_repeat.sh 3 1000000 42
```

The script invokes `./scripts/benchmark_release.sh` once per repetition with clear run headings. It does not compute statistics automatically — review the printed phase metrics and pick median or typical values yourself.

**Why repeated runs matter:** A one-off fast run can overstate gains; a one-off slow run can suggest a regression that does not reproduce. Milestone 6C baseline tables used single before/after runs — treat them as directional only until repeated runs confirm the trend.

**Debug vs Release:** Never compare Debug build output to Release output when claiming throughput improvements. Always use `./scripts/benchmark_release.sh` or `benchmark_repeat.sh` for performance statements.

**Avoid overclaiming:** Document machine, compiler, command count, seed, and build type. Do not present local synthetic benchmark throughput as production exchange latency. See [PROFILING_REPORT.md](PROFILING_REPORT.md) for profiling tools and next optimisation candidates.

## Sample results (machine-dependent)

Observed on a local **Apple Clang / Release-style** build (not a guarantee on your hardware):

### 1,000,000 commands, seed 42

| Metric | Approximate range |
|--------|-------------------|
| Trades | ~562,281 |
| Runtime | ~1.08s – 1.13s |
| Throughput | ~886k – 928k commands/sec |
| Latency avg | ~953 – 1005 ns |
| Latency min | ~83 – 84 ns |
| Latency p50 | ~875 ns |
| Latency p95 | ~2167 – 2250 ns |
| Latency p99 | ~3000 – 3375 ns |
| Latency max | ~999,250 – 1,053,375 ns |

### 10,000 commands, seed 42 (quick smoke)

Often ~500k+ commands/sec with proportionally fewer trades; use for fast regression only.

## Binary protocol benchmark output format

Example format (values are intentionally illustrative):

```text
Benchmark: Binary protocol replay
Commands: 100000
Seed: 42
Binary file: /tmp/cpp_low_latency_orderbook_binary_protocol_42.obk
Binary file size: 6400000 bytes
Trades: <machine-dependent>
Active orders: <machine-dependent>
Resting quantity: <machine-dependent>
Phases:
  command generation: <seconds>s
  binary write: <seconds>s (<commands/sec> commands/sec, <ns/command> ns/command)
  buffered read/decode: <seconds>s (<commands/sec> commands/sec, <ns/command> ns/command)
  buffered engine apply: <seconds>s (<commands/sec> commands/sec, <ns/command> ns/command)
  buffered read/decode + apply total: <seconds>s (<nanoseconds> ns)
  streaming read/decode/apply: <seconds>s (<commands/sec> commands/sec, <ns/command> ns/command)
  buffered total minus streaming total: <nanoseconds> ns
  total measured phases: <seconds>s (<nanoseconds> ns)
Note: metrics are local wall-clock timings; p50/p95/p99 are not reported because this benchmark measures phase totals, not per-command decode latencies. Positive buffered-minus-streaming means streaming was faster on this run; negative means buffered was faster.
```

**Important:**

- Results vary by CPU, compiler, build type, and system load.
- Use the **same machine and build flags** for before/after comparisons.
- Do **not** cite these numbers as production exchange latency.
- Future optimisation (ring buffer, struct layout, file I/O strategy) should be **benchmark-driven** with documented methodology here.

## Interpreting latency spikes

- **max** often reflects occasional allocation, vector growth, or OS effects—not typical steady-state.
- **p50** is usually more stable than **avg** for skewed distributions.
- Compare **p95/p99** when evaluating tail latency improvements.

## Related tests

- `WorkloadGeneratorTest.GeneratesDeterministicCommandsForFixedSeed` — same seed → same commands
- `WorkloadGeneratorTest.RejectsInvalidMix` — config validation
- `WorkloadGeneratorTest.GeneratedCommandsRoundTripThroughBinaryFile` — generated commands can be written/read as OBK1 messages

## Future benchmark work (not implemented)

- Ring-buffer pipeline vs direct call (Milestone 6)
- Separate CSV parse benchmark vs engine hot path

See [ROADMAP.md](ROADMAP.md).
