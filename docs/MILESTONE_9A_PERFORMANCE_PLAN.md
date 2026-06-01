# Milestone 9A — Performance Baseline and Hot-Path Optimisation Plan

**Status:** Complete (planning and measurement only; no engine/book/protocol code changes).  
**Date:** 2026-06-01  
**Git:** `f2822d1` on branch `feature/6f-memory-pool`  
**Machine:** macOS 26.5 (25F71), Apple Silicon ARM64, Release (`build-release/`), local wall-clock benchmarks.

This document is the 9A deliverable. Implementation belongs in **9B** only if acceptance criteria below are met.

---

## 1. Executive summary

| Finding | Detail |
|---------|--------|
| **Dominant cost today** | **Matching engine + order book apply** on synthetic and binary-replay workloads — not binary decode or OBK1 write. |
| **Binary path** | Read/decode ~20M cmd/s vs buffered engine apply ~11.6M cmd/s on the same 100k/seed-42 run — apply is ~1.7× slower than decode alone. |
| **Residual hotspot (profiler)** | Resting adds in `OrderBook::add_order_to_side`: **`std::list<Order>::push_back`** node allocations strongest `operator new` signal; hash emplace and map price-level inserts still visible after 6G `reserve_active_orders`. |
| **Already optimised (do not repeat in 9B)** | Cancel duplicate lookup (6E), `process_into` event buffer reuse (6F), `order_lookup_` reserve (6G slice 1). |
| **Rejected without new byte evidence** | `std::list` / `std::map` family swap, multithreaded SPSC for throughput, persistence/TCP/publisher. |
| **Recommended 9B target** | **Slice 1:** byte-ranked allocation profile (Instruments Allocations or Linux `heaptrack`) on engine-only apply loop. **Slice 2 (conditional):** one narrow change targeting **list node allocation on resting adds** only if profiling confirms it dominates apply-loop heap traffic. |

---

## 2. Dated Release baseline (2026-06-01)

Commands unless noted: **100 000**, seed **42**, `./scripts/verify.sh` passing (**164/164**) before runs.

### 2.1 `./scripts/benchmark_release.sh 100000 42`

| Phase | Throughput | ns/command | Notes |
|-------|------------|------------|--------|
| Binary write (OBK1 encode + file) | 16.37M cmd/s | 61 | I/O + encode |
| Buffered read/decode | 20.03M cmd/s | 49 | Decode only |
| Buffered engine apply | 11.65M cmd/s | 85 | `process_into` hot path |
| Streaming read/decode/apply | 7.95M cmd/s | 125 | Per-message apply |
| Matching engine synthetic (`process_into`) | 9.39M cmd/s | avg 89, **p50 83** | Same trade/book counts |

**Sanity (unchanged across paths):** 56 086 trades; 10 070 active orders; 5 039 585 resting quantity. Buffered and streaming trade/book totals match.

Raw log (local): `/tmp/bench_9a_100k_42.txt` — not committed.

### 2.2 Matching engine — three consecutive runs (same session)

| Run | Throughput | p50 (ns) |
|-----|------------|----------|
| 1 | 8.92M cmd/s | 83 |
| 2 | 9.30M cmd/s | 83 |
| 3 | 9.52M cmd/s | 83 |
| **Typical** | **~9.3M cmd/s** | **83** |

Spread ~6% without code changes — treat single-run tables as directional.

### 2.3 SPSC pipeline — `./build-release/ring_buffer_pipeline_benchmark 100000 42`

| Path | Throughput | vs direct |
|------|------------|-----------|
| Direct `process_into` | ~7.58M cmd/s | baseline |
| SPSC `run_sequence` | ~6.42M cmd/s | **~15% slower** on this run |

Equivalence sanity passed (same events/book metrics). **Do not** use SPSC for single-threaded throughput wins.

### 2.4 Engine-only harness (profiling orientation)

```bash
./build-release/matching_engine_benchmark 100000 42 --profile-engine-only
```

One run reported **~10.2M cmd/s** on the apply loop only (harness without per-command latency instrumentation in that phase). Use for profiling timing, not for cross-doc throughput claims.

### 2.5 Historical context (same machine family, older commits)

Documented medians after 6F/6G in [PERFORMANCE_BASELINE.md](PERFORMANCE_BASELINE.md): matching engine **~9–10M cmd/s**, buffered apply **~11–12M cmd/s**. Post-8B **HEAD** is in the same band — no regression inferred from one 9A session.

---

## 3. Cost separation (where time goes)

| Subsystem | Evidence | Dominates? | Confidence |
|-----------|----------|------------|------------|
| **Matching engine + order book apply** | Buffered apply 11.6M vs decode 20.0M; ME benchmark ~9.3M; 6E–6G engine-only `sample` | **Yes** for end-to-end replay | **High** |
| **Binary parse / OBK1 decode** | ~20M cmd/s buffered decode | No vs apply | **High** |
| **Binary encode / file write** | ~16M cmd/s | No vs apply | **High** |
| **Event vector allocation** | 6F `process_into` in benchmarks; weak alloc signal in 6G apply `sample` | Largely mitigated on hot benchmarks | **Medium** |
| **`order_lookup_` rehash** | Reduced after 6G reserve; residual emplace in 6G slice 2 | Secondary on adds | **Medium** |
| **List node alloc (`push_back`)** | 6G slice 2: **8** `operator new` stacks under list offset | **Leading alloc candidate** on resting adds | **Medium** (time-sample, not bytes) |
| **Map price-level insert** | 1–2 `operator new` stacks on adds | Smaller than list on adds | **Medium** |
| **Match/cancel teardown** | `remove_order_at_location` top collapsed CPU stacks | CPU + free heavy; not single alloc winner | **Medium** |
| **SPSC / pipeline** | 6H + 9A rerun: slower than direct | **Not** for single-thread perf | **High** |
| **Replay visualiser / NDJSON export** | Not in C++ benchmarks; separate Node build | Out of 9A hot-path scope | **High** (scope) |
| **Per-command `steady_clock` in ME benchmark** | Present in default benchmark path | Adds noise; masks pure throughput | **Medium** |

**Replay path bottleneck (100k workload):** engine apply, not decode. Streaming is slower than buffered apply on this run (7.95M vs 11.65M) due to per-message dispatch overhead — not a correctness issue.

---

## 4. Profiling summary (9A session + carried-forward 6G)

### 4.1 Methodology (unchanged)

See [PROFILING_REPORT.md](PROFILING_REPORT.md). Engine-only mode:

```bash
./build-release/matching_engine_benchmark 1000000 42 --profile-engine-only
# During countdown "1…", in another terminal:
sample <pid> 12 -file profiling/9a/engine_only_sample_9a_loop.txt
```

Store traces under `profiling/` (gitignored). Do not commit raw `.trace` bundles.

### 4.2 9A `sample` caveat

`profiling/9a/engine_only_sample_9a.txt` was captured **without** waiting for the apply-loop countdown — stacks show `sleep_for` / `nanosleep` only. **Do not** use that file for hotspot ranking.

A follow-up sample during a 500k apply window **missed** the process (loop finished in &lt;50 ms). **Primary profiler evidence for 9A remains 6G slice 2** (`profiling/6g/allocation_attribution.txt`, `allocation_engine_loop_sample.txt`) on the same codebase family.

### 4.3 Top measured hotspots (ranked for 9B planning)

| Rank | Area | Function / site | Evidence | Category | Worth optimising? |
|------|------|-----------------|-----------|----------|-------------------|
| 1 | Order book | `add_order_to_side` → `std::list<Order>::push_back` | 6G apply `sample`: 8× `operator new` under list offset | Book / **allocation** | **Yes**, after byte-ranked confirm in 9B |
| 2 | Order book | `order_lookup_` emplace (post-reserve) | 4+3+1 alloc stacks on adds | Book / allocation | **Maybe** — only if 9B alloc trace shows material bytes |
| 3 | Order book | `buy_book_` / `sell_book_` map insert | `__tree_balance_after_insert`, fewer samples than list | Book / allocation | **Defer** unless alloc trace ranks it |
| 4 | Order book | `remove_order_at_location` | 11 collapsed stacks on match/cancel | Book / CPU + free | **Defer** — mixed teardown; no single alloc winner |
| 5 | Matching engine | `execute_order` / match loop | Visible in 6E engine-only samples | Engine + book | **Defer** until add-path alloc reduced |
| 6 | Harness | `WorkloadGenerator::generate` | Dominates non-engine-only `sample` | **Not engine** | No — use `--profile-engine-only` |
| 7 | Harness | Per-command latency clocks in `matching_engine_benchmark` | Architectural overhead | Measurement | **Optional** later benchmark mode (roadmap §5) |

---

## 5. Data-structure review (no rewrite in 9A)

| Structure | Role | 9A conclusion |
|-----------|------|----------------|
| `std::list<Order>` per price | Stable iterators for `OrderLocation` | Keep; alloc cost on `push_back` is the **hypothesis** for 9B, not a swap |
| `std::map` price levels | Ordered best bid/ask | Keep; weak alloc signal vs list on adds |
| `std::unordered_map` `order_lookup_` | O(1) cancel/reduce | Reserve done (6G); residual emplace acceptable until byte proof says otherwise |
| `process_into` + scratch `vector<EngineEvent>` | Output reuse | Hot benchmarks already use it; `process()` still allocates for compatibility |

---

## 6. Optimisation candidates (ranked)

| # | Candidate | Risk | 9A verdict |
|---|-----------|------|------------|
| A | Byte-ranked alloc profile (Instruments / `heaptrack`) on engine-only 2M+ commands | Low (measurement) | **Do first in 9B** |
| B | List node pool / arena for resting orders only (same `std::list` interface) | Medium–high | **9B implementation** if A confirms list dominates bytes |
| C | Further `order_lookup_` / map tuning | Low–medium | Only if A ranks hash/map above list |
| D | `std::deque` or intrusive list instead of `std::list` | High | **Rejected** until A + full invariant tests |
| E | `flat_map` / sorted vector for price levels | High | **Rejected** (6G lesson) |
| F | SPSC for single-thread throughput | N/A | **Rejected** (measured slower) |
| G | Benchmark throughput mode (no per-command clock) | Low | **Optional** 9B alt if A inconclusive — improves measurement, not production path |
| H | TCP gateway, persistence, publisher | N/A | **Out of scope** (8C decision) |

---

## 7. Recommended 9B target

### Primary recommendation

**9B — Resting-order list node allocation (conditional two-slice milestone)**

1. **Slice 1 (measurement, required):** Engine-only apply loop, 1M–2M commands, seed 42, `reserve_book_capacity` enabled. Capture **allocation counts or bytes** with Instruments Allocations (macOS) or `heaptrack` (Linux CI/dev box). Success criterion: documented table ranking `list push_back`, hash emplace, map insert.

2. **Slice 2 (implementation, only if slice 1 shows list node alloc ≥ ~40% of apply-loop heap OR clearly dominant `operator new` site):** One narrow change — e.g. **price-level list node pool** or session-local arena backing existing `std::list` nodes — **without** changing FIFO, cancel iterators, or matching rules.

### Why this target

- Release benchmarks show **engine apply** is the replay bottleneck; profiler time-samples already point at **`add_order_to_side`** and list `push_back` alloc stacks (6G slice 2).
- 6F removed event-vector churn; 6G removed most hash rehash; **list nodes** are the largest **remaining** measured alloc signal on adds.
- Narrower than map/list container family swap; aligned with [PERFORMANCE_ROADMAP.md](PERFORMANCE_ROADMAP.md) §1–2.

### Alternate 9B (if slice 1 is inconclusive)

**Benchmark harness throughput mode** — flag to disable per-command `steady_clock` in `matching_engine_benchmark` for stable cmd/s comparisons. Low risk; does not change production semantics. Use to validate future book changes.

---

## 8. 9B acceptance criteria

- [ ] `./scripts/verify.sh` — **164/164** (or current count) passes.
- [ ] No change to matching priority, trade counts, rejection strings, OBK1 layout, or CSV/binary equivalence tests.
- [ ] If code changes book storage: run `test_matching_engine_workload_invariants` and FIFO/priority tests; document iterator/lifetime proof for any new allocator.
- [ ] Repeated Release benchmarks: `./scripts/benchmark_repeat.sh 5 100000 42` — report **medians** for matching engine and buffered engine apply; state if improvement is within noise.
- [ ] Update [PERFORMANCE_BASELINE.md](PERFORMANCE_BASELINE.md) and [PROFILING_REPORT.md](PROFILING_REPORT.md) with dated, machine-labelled results — no portable throughput claims.
- [ ] If slice 1 does not confirm list dominance: **stop** slice 2; document “no change warranted” and advance queue per human review.

---

## 9. Out of scope (9B and beyond)

- Persistence / command journal, TCP gateway, market data publisher (8C deferred).
- `std::list` / `std::map` wholesale replacement without byte-ranked proof.
- Multithreaded SPSC as a performance strategy.
- Binary protocol v1 layout changes.
- CI benchmark regression gates (optional backlog).
- Replay visualiser feature work.
- Committing `profiling/` artifacts.

---

## 10. 9B slice 1 outcome (2026-06-01)

| Question | Answer |
|----------|--------|
| List nodes dominate bytes? | **No** — byte counts unavailable; time-samples show **hash emplace ≥ list `push_back`** at 2M commands |
| Slice 2 justified? | **No** — stop per §8 (“no change warranted”) |
| Event vector churn? | **Low** signal in apply-window sample |
| Raw traces | `profiling/9b/` (gitignored) |

See [PROFILING_REPORT.md](PROFILING_REPORT.md) §9B slice 1.

---

## 11. Commands run (9A)

```bash
git push origin feature/6f-memory-pool   # succeeded f2822d1 (prior session)

./scripts/benchmark_release.sh 100000 42
./build-release/matching_engine_benchmark 100000 42   # ×3
./build-release/ring_buffer_pipeline_benchmark 100000 42
./build-release/matching_engine_benchmark 100000 42 --profile-engine-only
sample <pid> …   # mistimed capture → profiling/9a/engine_only_sample_9a.txt (invalid)
```

Validation (post-docs):

```bash
git diff --check
./scripts/verify.sh
cd ui/replay-visualiser && npm run build
```

---

## 11. References

- [MILESTONE_8C_DECISION.md](MILESTONE_8C_DECISION.md)
- [PERFORMANCE_BASELINE.md](PERFORMANCE_BASELINE.md) §9A
- [PROFILING_REPORT.md](PROFILING_REPORT.md) §9A
- [PERFORMANCE_ROADMAP.md](PERFORMANCE_ROADMAP.md)
- [BENCHMARKING.md](BENCHMARKING.md)
