# Performance Experiments

Local, machine-dependent results. Experiments live on isolated branches; do not merge without review.

## experiment/vector-price-levels — vector price levels with tombstones

| Field | Value |
|-------|--------|
| **Branch** | `experiment/vector-price-levels` |
| **Baseline commit** | `846712e` (pre-change snapshot on branch tip) |
| **Experiment commit** | (see branch tip after prototype commit) |

### Motivation

Reduce `std::list<Order>` node allocations, pointer chasing, and list erase cost on fills/cancels by using contiguous per-price storage with index-based `OrderLocation` and lazy tombstones.

### Old structure

- `std::map<price, std::list<Order>>` per side
- `OrderLocation` stored `std::list<Order>::iterator`
- Cancel/fill: `list::erase` at iterator; empty level removed from map

### New structure

- `std::map<price, PriceLevel>` where `PriceLevel` holds `std::vector<OrderSlot>` + `head` index
- `OrderSlot { Order order; bool active; }`
- `OrderLocation { side, price, std::size_t index }` — no stored iterators
- Resting orders: append to vector back (FIFO)
- Cancel / full fill: mark inactive, advance `head` over tombstones, remove map entry when no active orders
- Partial fill: decrement quantity in slot

### Correctness

- `./scripts/verify.sh`: **172/172 passed** (167 existing + 5 focused `OrderBookTest`s)
- Synthetic workload totals (seed 42, 100k): **Trades 56086**, **Active orders 10070**, **Resting quantity 5039585** — match list baseline

### Benchmark (throughput-only, 5×100k, seed 42, Release)

| Phase | Baseline (`846712e`) | Experiment (vector levels) | Delta |
|-------|----------------------|----------------------------|-------|
| Matching engine | **11,312,697** cmd/s | **~9,602,804** cmd/s (manual 5-run median) | **~−15%** (clear regression) |
| Buffered engine apply | 11,196,954 cmd/s | (not re-snapshot; expect similar direction) | — |
| 2M single run | ~8.3M cmd/s (prior sessions) | **8,066,092** cmd/s | slower |

**Verdict:** **Slower — abandon for production.** Regression exceeds noise (>5%). Likely causes: scanning tombstones from `head` on `peek_best_*` / matching, vector slot size vs list node locality, extra branches on `active`.

### Tradeoffs observed

- **Pros:** No list node alloc per order; stable index lookup; simpler mental model for pools later
- **Cons:** Tombstone scanning on hot peek/match path; vectors retain dead slots until level drained; `peek_best_*` walks map + scans level

### Recommendation

**Abandon merge.** Keep branch for reference or delete after review. Do not port to `feature/6f-memory-pool` without a redesign (e.g. periodic compaction, separate active index queue, or slab per level).

**Hardening:** If revisiting, use a stronger model / dedicated perf pass — compaction strategy and cache-line layout need design before another benchmark attempt.
