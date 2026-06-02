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
| Buffered engine apply | ~8–12M commands/sec |
| Streaming read/decode/apply | ~6–8M commands/sec |
| Matching engine synthetic loop | ~9–10M commands/sec, p50 ~83 ns |

**9A dated snapshot (2026-06-01):** see [PERFORMANCE_BASELINE.md](PERFORMANCE_BASELINE.md) §9A.

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

## 6G allocation attribution (slice 2 — measurement only)

**Scope:** No production code changes. Local macOS only; no throughput or speedup claims.

| Tool | Outcome |
|------|---------|
| `malloc_history` + `MallocStackLogging` | Dominated by harness `WorkloadGenerator::generate` → `vector<OrderCommand>::reserve` (~64 MiB). Apply loop (~0.11 s) too short for live history to attribute book-path allocs. |
| `xctrace record --template Allocations` | Attach failed on this host; trace not analysed. `heaptrack` not installed. |
| `sample` during apply loop | Primary evidence. After countdown `1…`, `sample <pid> 12` → `profiling/6g/allocation_engine_loop_sample.txt` (not in git). Summary also in `profiling/6g/allocation_attribution.txt`. |

**Offset map** (`atos` on `build-release/matching_engine_benchmark`, load address from sample): in `add_order_to_side`, `+204` = `std::list<Order>::push_back`, `+128` / `+388` = `std::map` price-level insert, `+544` / `+644` = `order_lookup_` hash emplace.

**Apply-loop `sample` (directional, not byte counts):**

| Site | Signal in apply window |
|------|-------------------------|
| List `push_back` (`+204`) | **8** stacks with `operator new` directly under this offset — strongest alloc signal on resting adds |
| Hash `order_lookup_` emplace | **4+3+1** stacks with `operator new` under emplace — residual after slice 1 reserve |
| Map new price level (`__tree_balance_after_insert`) | **1–2** stacks on add path |
| `remove_order_at_location` | **11** collapsed stacks (match/cancel teardown; mix of free + map/list/hash work) |
| `process_into` / `EngineEvent` vector | Dispatch visible; no strong per-command output-vector alloc (6F reuse) |
| Harness command vector | Outside apply loop; dominates `malloc_history` lifetime totals |

**Cannot claim:** allocation bytes or counts per site; a single dominant allocator family. That would need Instruments Allocations or `heaptrack` on a machine where attach works.

**Slice 2 conclusion:** Allocation pressure on resting adds is **mixed** (list nodes, hash inserts, occasional map price-level nodes). Match/cancel teardown is hot but does not isolate one alloc winner. **Does not justify** a `std::list` or `std::map` container swap from this evidence.

**Recommendation:** Treat **6G book-structure work as complete** for human review; advance the milestone queue when agreed. No container replacement in slice 2. Optional future work: Instruments Allocations or Linux `heaptrack` if a byte-ranked profile is needed before any list/map experiment.

## 9A profiling refresh (2026-06-01, post-8B HEAD)

**Scope:** Measurement and documentation only — no production code changes.

| Step | Result |
|------|--------|
| Baselines | `./scripts/benchmark_release.sh 100000 42` — see [PERFORMANCE_BASELINE.md](PERFORMANCE_BASELINE.md) §9A |
| Engine-only harness | `./build-release/matching_engine_benchmark 100000 42 --profile-engine-only` |
| `sample` during apply | **Invalid capture:** `profiling/9a/engine_only_sample_9a.txt` taken during countdown sleep, not apply loop. **Do not use for hotspots.** |
| Retry | 500k-command window too short for reliable `sample` attach on this machine |
| **Primary evidence** | **6G slice 2** unchanged on same codebase: `profiling/6g/allocation_attribution.txt`, apply-loop `sample` — list `push_back` strongest `operator new` on adds; mixed map/hash; `remove_order_at_location` hot on teardown |

**9A conclusion:** Hot-path CPU/allocation signals still centre on **`OrderBook::add_order_to_side`** (list nodes, residual hash emplace, occasional map level insert) and match/cancel teardown. Binary decode remains comparatively cheap vs engine apply (benchmark phase split).

**9B profiling prerequisite:** Byte-ranked allocation profile (Instruments Allocations or `heaptrack`) on engine-only 1M–2M command apply loop before any list-node pool implementation. Details: [MILESTONE_9A_PERFORMANCE_PLAN.md](MILESTONE_9A_PERFORMANCE_PLAN.md).

## 9B slice 1 — allocation profile (2026-06-01)

**Scope:** Measurement only — **no** engine/book code changes. **Git:** `9dc5298` + docs commit pending.

**Harness:** `./build-release/matching_engine_benchmark 2000000 42 --profile-engine-only` with `reserve_book_capacity(command_count / 10)`. Apply loop ~0.25s; 195 873 peak active orders.

**Tools used (macOS 26.5, ARM64):**

| Tool | Result |
|------|--------|
| `heaptrack` | Not installed |
| `xctrace record --template Allocations` | Recording ended with errors; `profiling/9b/allocations.trace` saved locally, not parsed |
| `malloc_history -callTree` | Lite MSL only; no useful call tree before process exit |
| **`sample` during apply** | **Primary evidence** — `profiling/9b/apply_loop_sample.txt` (15s window from countdown `1…`; includes some post-loop validation) |
| `MallocStackLogging=1` | Enabled for sample run; adds overhead; not byte-ranked |

**`atos` map (`add_order_to_side`):** `+128` map `operator[]`, `+204` `list::push_back`, `+408` map tree insert, `+544` / `+644` `order_lookup_` hash `emplace`.

**`operator new` time-samples under resting adds (apply window, directional):**

| Site | Approx. samples | Category |
|------|-----------------|----------|
| Hash emplace `+644` | ~58 | `order_lookup_` |
| Hash emplace `+544` | ~51 | `order_lookup_` |
| List `push_back` `+204` | ~46 | `std::list<Order>` node |
| Map tree `+408` | ~36 | Price-level `std::map` node |
| Map path `+128` | ~29 | Price-level lookup/insert |
| `process_into` / event vector | ~11 collapsed | **Low** — 6F reuse holds |
| Harness command vector | Outside apply loop | Dominates lifetime heap (~128 MiB for 2M cmds) |

**Compared to 6G (1M commands):** 6G slice 2 ranked **list `+204` strongest** on a shorter apply window. At **2M** commands, **hash emplace samples meet or exceed list** in this session — still **mixed**, not list-only.

**Confidence:** **Medium** for “alloc pressure is mixed on adds”; **low** for byte-level ranking (no Instruments/`heaptrack` bytes).

**Slice 2 decision:** **Not justified.** List nodes do **not** clearly dominate; hash and map allocations remain material. **Do not** implement a list node pool/arena from this evidence. Optional follow-ups (future milestones / Linux `heaptrack`): workload-shaped hash tuning, benchmark throughput mode, or byte-ranked Instruments on a repeated apply loop.

## 2M engine-only profiling refresh (2026-06-02, commit `5b639d0`)

**Scope:** Measurement and documentation only — **no** production or container changes.

| Item | Value |
|------|--------|
| Command | `./build-release/matching_engine_benchmark 2000000 42 --profile-engine-only` |
| Commands / seed | 2 000 000 / 42 |
| Mode | `--profile-engine-only` (generate once, 10s countdown, apply loop only) |
| Build | Release (`build-release`) |
| Profiler | macOS `sample` (time samples, **not** byte-ranked) |
| Apply runtime | ~0.24–0.28s (~7.1–8.3M cmd/s this session) |
| Peak active orders | 195 873 |

**Raw traces (gitignored, uncommitted):**

| File | Notes |
|------|--------|
| `profiling/current/engine_only_2m_apply_sample.txt` | **Valid** — `sample` started when log shows `  1...` (last countdown second + apply) |
| `profiling/current/engine_only_2m_sample.txt` | **Invalid** — captured `WorkloadGenerator::generate` / `memmove` (attached too early) |
| `profiling/current/benchmark_2m_apply.log` | Harness stdout |
| `profiling/current/profiling_summary.txt` | Local notes |

**Suggested attach (macOS):**

```bash
./build-release/matching_engine_benchmark 2000000 42 --profile-engine-only > profiling/current/run.log 2>&1 &
BPID=$!
while ! grep -q '^  1\.\.\.$' profiling/current/run.log; do sleep 0.02; done
sample "$BPID" 3 -file profiling/current/engine_only_2m_apply_sample.txt
wait "$BPID"
```

**Top hotspots (apply window, directional sample counts in call tree):**

| Area | What showed up | Interpretation |
|------|----------------|----------------|
| Resting add | `execute_new_order` → `add_order` → `add_order_to_side` | Still central hot path |
| Hash | `order_lookup_` `__emplace_unique_key_args` + `operator new` (e.g. +540, +268) | **Material** on every new resting order |
| List | `push_back` paths + `operator new` (+212, +444) | **Material** list-node alloc |
| Map | `operator[]` / tree insert / `__tree_balance_after_insert` (+128, +408, +424) | **Material** on new price levels |
| Match / fill | `match_*_limit` → `execute_order` → `remove_order_at_location` | Teardown + map erase + hash remove on fills |
| Cancel | `process_cancel` → `cancel_order` → `remove_order_at_location` | Visible; hash remove + frees |
| Engine checks | `process_new_order` → `contains_order` | Duplicate-ID hash lookup before add |
| Events | `process_into` / `EngineEvent` vector | Present but **not** top alloc stack in this capture |
| Harness | Command vector generation | Outside apply loop; dominates if `sample` attaches too early |

**Compared to 9B slice 1 (2026-06-01, same 2M harness):** Conclusion unchanged — **mixed** hash + list + map on adds; match/cancel teardown hot. Resting `Order` move (`5b639d0`) did not remove list-node `operator new` under `push_back`.

**Confidence:** **Medium** for mixed add-path allocation families and teardown on match/cancel; **low** for byte-ranked ranking or claiming one site wins (no Instruments/`heaptrack` bytes this pass).

**Next single optimisation milestone (recommended):** **Bounded `order_lookup_` hash tuning** (reserve/load-factor experiments on an isolated branch) — hash emplace + `operator new` remain prominent on every rest; list-node pool / container swap still **not** justified. Validate with repeated throughput-only medians + optional byte-ranked alloc tool before claiming a win.

## Resting-order copy removal (2026-06-02)

**Change:** `OrderBook::add_order(Order)` by value; list insert uses `push_back(std::move(order))`; engine path passes resting rvalue into `add_order`.

**Throughput-only median (5×100k, seed 42):** ~11.44M → ~11.30M cmd/s on one local session — **within noise**, not a measured win. List-node allocation on insert remains; next step remains byte-ranked profiling (see [PERFORMANCE_OPTIMISATION_BACKLOG.md](PERFORMANCE_OPTIMISATION_BACKLOG.md)).

Local summary (not in git): `profiling/9b/allocation_summary.txt`.

## Future optimisation work

Profiler-backed candidates, risk notes, and the required milestone process are documented in **[PERFORMANCE_ROADMAP.md](PERFORMANCE_ROADMAP.md)** (section *Future Performance Optimisation Candidates*).

**Recently completed:** **9B slice 1** allocation profile (mixed list/hash/map; **no** slice 2 implementation). **9A** baseline + plan. **Next:** human review; consider advancing queue without list-node pool, or a future measurement-only / hash-tuning milestone — not a container swap.

## Related documentation

- [PERFORMANCE_ROADMAP.md](PERFORMANCE_ROADMAP.md) — future candidates and optimisation process
- [BENCHMARKING.md](BENCHMARKING.md) — harness details and repeated-run workflow
- [PERFORMANCE_BASELINE.md](PERFORMANCE_BASELINE.md) — recorded local baselines and 6C comparison
- [ARCHITECTURE.md](ARCHITECTURE.md) — OrderBook vs MatchingEngine boundaries
