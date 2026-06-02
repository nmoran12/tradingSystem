# Performance Tracking

This document describes the local benchmark history workflow. Results are **machine-dependent** and should only be compared against repeated Release runs on the same machine, build type, command count, and seed.

## Record a snapshot

From the repository root:

```bash
./scripts/record_benchmark_snapshot.sh 5 100000 42
```

Arguments are:

1. run count, default `5`
2. command count, default `100000`
3. workload seed, default `42`

The script:

1. runs `./scripts/verify.sh`
2. runs repeated Release benchmarks via `./scripts/benchmark_repeat.sh`
3. saves the raw transcript under `benchmark-history/raw/`
4. appends one JSONL summary to `benchmark-history/benchmark_history.jsonl`
5. regenerates `benchmark-history/dashboard.html`

If verification fails, the script exits non-zero before recording benchmark results.

## History files

```text
benchmark-history/
  benchmark_history.jsonl
  raw/
    <timestamp>_<commit>_benchmark.txt
  dashboard.html
```

`benchmark_history.jsonl` is append-only. Do not rewrite older entries when adding a new snapshot. Raw logs are kept so parsed numbers can be checked against benchmark output later.

The JSONL metrics are medians across the repeated runs when a metric is parsed successfully. Missing or unparseable metrics are stored as `null`.

## Open the dashboard

Open this file directly in a browser:

```text
benchmark-history/dashboard.html
```

The dashboard is static HTML/CSS/JavaScript with embedded JSON data regenerated from `benchmark_history.jsonl`. It does not require a server, backend, authentication, or external dependencies.

## Metrics that matter most

- **Matching engine throughput**: direct synthetic `MatchingEngine` command loop throughput.
- **Buffered engine apply throughput**: decoded OBK1 vector applied through `MatchingEngine`.
- **Streaming read/decode/apply throughput**: one OBK1 message decoded and applied at a time.
- **p50/p95/p99 latency**: matching-engine per-command latency distribution from `matching_engine_benchmark`.

Binary write and buffered read/decode remain useful context, but existing evidence says engine apply / order-book work is the more important optimisation target than OBK1 decode.

## Compare before and after

Use the same:

- machine and OS
- compiler/toolchain
- Release build
- command count
- seed
- run count

Prefer repeated-run medians. Do not claim an improvement from one lucky run, especially if the before/after ranges overlap.

## What not to claim

- Do not claim production exchange latency.
- Do not claim portable performance across machines.
- Do not compare Debug numbers with Release numbers.
- Do not claim an optimisation improved performance unless repeated medians improve beyond normal run-to-run noise.

## Related docs

- [BENCHMARKING.md](BENCHMARKING.md)
- [PERFORMANCE_BASELINE.md](PERFORMANCE_BASELINE.md)
- [PROFILING_REPORT.md](PROFILING_REPORT.md)
- [PERFORMANCE_OPTIMISATION_BACKLOG.md](PERFORMANCE_OPTIMISATION_BACKLOG.md)
