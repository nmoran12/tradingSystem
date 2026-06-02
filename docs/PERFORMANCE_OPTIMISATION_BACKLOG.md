# Performance Optimisation Backlog

This backlog records future optimisation ideas. It is intentionally conservative: no matching semantics, FIFO priority, OBK1 format, CSV/binary equivalence, or replay visualiser behaviour should change for any item here.

## Current evidence summary

- Release benchmark phase split shows engine apply / order-book work is the main bottleneck, not OBK1 decode.
- Single benchmark runs are noisy; repeated-run medians matter.
- Completed work already removed duplicate cancel lookup, added caller-owned `process_into` event buffer reuse, reserved `order_lookup_`, and measured SPSC as slower for single-thread throughput.
- Allocation profiling around `OrderBook::add_order_to_side` is mixed: hash emplace, list `push_back`, and map inserts all appear. There is not enough evidence for a list pool, arena, `std::list` replacement, `std::map` replacement, or broad container rewrite.
- **Done (no clear win):** resting `Order` move into `std::list` via `add_order(Order)` + `push_back(std::move)` — throughput-only median ~11.30M vs ~11.44M cmd/s before (noise).

## Ranked candidates

| Rank | Candidate | Suspected bottleneck | Current evidence | Extra measurement needed | Safe to implement now? | Required validation |
|------|-----------|----------------------|------------------|--------------------------|------------------------|--------------------|
| 1 | Add benchmark throughput-only mode | Per-command `steady_clock` timing adds noise when measuring aggregate throughput | **Done** — `--throughput-only` on `matching_engine_benchmark`; snapshot script and dashboard support | Compare throughput-only medians before/after future engine changes | **Done (measurement infra)** | `./scripts/verify.sh`; repeated Release runs; same trades/book totals |
| 2 | Improve benchmark parsing/reporting | Manual comparison is error-prone | New JSONL tracker parses existing output but benchmark binaries still print text only | Validate parser against several existing raw logs; consider optional machine-readable benchmark output later | **Yes, tooling only** | Parser fixture tests if added; `git diff --check`; dashboard generation |
| 3 | Re-profile engine-only apply with 2M+ commands | Apply loop is short at 100k, hard to attach profilers reliably | **Done (2026-06-02)** — log-synced `sample` on 2M apply; mixed hash/list/map; see PROFILING_REPORT §2M refresh | Byte-ranked follow-up optional | **Done (measurement)** | Raw traces under `profiling/current/` (not committed) |
| 4 | Investigate `order_lookup_` hash tuning | Hash emplace remains visible after reserve | 9B samples showed hash emplace comparable to list node allocation | Byte/count-ranked allocation profile; try load factor/reserve experiments in an isolated branch | **Not yet** | Full tests; repeated Release medians; no order lookup behaviour changes |
| 5 | Investigate list/map allocation | Resting adds allocate list nodes and occasional map nodes | 6G/9B samples show mixed list/hash/map allocation pressure | Byte-ranked allocation data proving one allocator site dominates | **No** | FIFO tests, workload invariants, CSV/binary equivalence, repeated Release benchmarks |
| 6 | Investigate symbol/string handling | Commands carry `std::string symbol` | Listed as possible hot area, but not proven dominant in current profiles | CPU/allocation profile showing symbol operations in hot stacks | **No** | Existing parser/protocol tests; benchmark before/after with same workload |
| 7 | Revisit SPSC for real producer/consumer experiments | Single-thread enqueue/drain overhead was slower | 6H measured SPSC slower than direct path for single-thread throughput | Only revisit with a real multi-threaded measurement goal, not current hot-loop claims | **No for single-thread throughput** | Equivalence tests; clear scope and no throughput claim unless measured |

## Profiling refresh (2026-06-02) — done

**Done:** 2M `--profile-engine-only` + log-synced `sample` on apply window (`profiling/current/engine_only_2m_apply_sample.txt`). See [PROFILING_REPORT.md](PROFILING_REPORT.md) §2M engine-only profiling refresh.

## Next three optimisation candidates (ranked)

| Candidate | Evidence | Risk | Expected benefit | Implement now? |
|-----------|----------|------|------------------|----------------|
| **1. Bounded `order_lookup_` hash tuning** | 2M apply `sample` (2026-06-02) and 9B: hash `emplace` + `operator new` on every rest; `contains_order` on new orders; hash remove on fill/cancel. Comparable to list alloc in 9B. `reserve_active_orders` already done. | **Low–medium** — wrong load factor/reserve can regress lookup or memory; must not change lookup semantics | Possible small–moderate throughput if rehash/emplace cost drops; **uncertain without byte-ranked proof** | **Yes — next single code milestone** (isolated branch; no container swap) |
| **2. Byte-ranked allocation profile (Instruments / `heaptrack`)** | Time-`sample` still mixed; cannot rank bytes. `xctrace` / `malloc_history` failed or were inconclusive in 9B. | **Low** (measurement only) | Unblocks confident choice between hash vs list vs map levers | **Yes if hash tuning inconclusive** — measurement-only |
| **3. Cancel/fill teardown micro-optimisation** | `remove_order_at_location` + map `__tree_remove` + hash `remove` visible on `execute_order` / `cancel_order` | **Medium** — easy to break FIFO/iterator stability | Unclear until add-path hash/list cost is reduced; spread across match+cancel | **No** — not first |

**Not recommended now:** list node pool, `std::list`/`std::map` replacement, arenas, intrusive containers (evidence still mixed; copy-move rest did not remove list `operator new`).

## Next recommended optimisation milestone

**Implement exactly one next milestone:** **bounded `order_lookup_` hash tuning** (e.g. peak-active-order reserve sizing from workload stats, `max_load_factor` experiments) on branch `feature/6f-memory-pool` or a short-lived child branch.

Why this one:

- Fresh 2M apply-window `sample` still shows hash emplace + allocation on every resting add alongside list/map.
- Lower risk than container replacement; aligns with existing `reserve_active_orders` work.
- Throughput-only benchmark mode exists for before/after medians.

**Before claiming a win:** run `./scripts/record_benchmark_snapshot.sh 5 100000 42 --throughput-only`; keep FIFO/cancel/workload tests green. Prefer Instruments Allocations or Linux `heaptrack` if hash tuning shows no median gain.

**Stronger model?** Only if the next change touches book invariants, iterator stability, or custom hash/equivalence types beyond load factor and reserve — otherwise Auto is sufficient.

Do **not** implement list pools, arenas, intrusive containers, map/list replacement, persistence, TCP, publisher, database, or live market data without new byte-ranked evidence.
