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
| 3 | Re-profile engine-only apply with 2M+ commands | Apply loop is short at 100k, hard to attach profilers reliably | 9B 2M run gave a longer apply window and mixed allocation signals | Fresh `sample`, Instruments Allocations, or Linux `heaptrack` on the same commit and workload | **Yes, measurement only** | No code changes; raw traces stay under gitignored `profiling/` |
| 4 | Investigate `order_lookup_` hash tuning | Hash emplace remains visible after reserve | 9B samples showed hash emplace comparable to list node allocation | Byte/count-ranked allocation profile; try load factor/reserve experiments in an isolated branch | **Not yet** | Full tests; repeated Release medians; no order lookup behaviour changes |
| 5 | Investigate list/map allocation | Resting adds allocate list nodes and occasional map nodes | 6G/9B samples show mixed list/hash/map allocation pressure | Byte-ranked allocation data proving one allocator site dominates | **No** | FIFO tests, workload invariants, CSV/binary equivalence, repeated Release benchmarks |
| 6 | Investigate symbol/string handling | Commands carry `std::string symbol` | Listed as possible hot area, but not proven dominant in current profiles | CPU/allocation profile showing symbol operations in hot stacks | **No** | Existing parser/protocol tests; benchmark before/after with same workload |
| 7 | Revisit SPSC for real producer/consumer experiments | Single-thread enqueue/drain overhead was slower | 6H measured SPSC slower than direct path for single-thread throughput | Only revisit with a real multi-threaded measurement goal, not current hot-loop claims | **No for single-thread throughput** | Equivalence tests; clear scope and no throughput claim unless measured |

## Next recommended optimisation milestone

The next single optimisation milestone should be **re-profile engine-only apply with 2M+ commands** (measurement only).

Why:

- Throughput-only benchmark mode is now available for cleaner before/after throughput comparisons.
- Current allocation evidence on `add_order_to_side` remains mixed; no container rewrite is justified yet.
- A longer apply window makes profiler attachment and byte-ranked allocation tools more reliable.

Do **not** implement list pools, arenas, intrusive containers, map replacement, persistence, TCP, publisher, database, or live market data without new byte-ranked evidence.
