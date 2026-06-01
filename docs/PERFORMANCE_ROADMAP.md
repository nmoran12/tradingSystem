# Performance Roadmap

Profiler-backed **future optimisation candidates** for this project. Nothing here is a committed change or a guaranteed improvement — each item needs fresh profiling, a narrow implementation, tests, and repeated Release benchmarks before it ships.

**Context after Milestone 6F:** `MatchingEngine::process_into(command, events)` lets hot loops reuse a caller-owned `std::vector<EngineEvent>` instead of constructing a fresh vector per command. That reduced event-output allocation pressure while `process()` remains a compatibility wrapper. The internal `OrderBook` storage model was **not** changed.

**Active milestone queue:** **9A** (plan complete) recommends **9B** — byte-ranked alloc profile, then **conditional** list-node allocation reduction in `add_order_to_side` if justified. See [MILESTONE_9A_PERFORMANCE_PLAN.md](MILESTONE_9A_PERFORMANCE_PLAN.md). **6G** book-structure work is closed (no container swap without new byte evidence).

See also: [PROFILING_REPORT.md](PROFILING_REPORT.md), [PERFORMANCE_BASELINE.md](PERFORMANCE_BASELINE.md), [BENCHMARKING.md](BENCHMARKING.md), [ACTIVE_MILESTONE.md](ACTIVE_MILESTONE.md).

---

## Future Performance Optimisation Candidates

### 1. `OrderBook::add_order_to_side` allocation pressure

**Profiler evidence (6E / 6F):** After `process_into`, engine-only `sample` still shows `add_order_to_side` as a major apply-loop cost. Stacks include:

- `std::list<Order>::push_back`
- `std::unordered_map` insert/emplace and possible rehashing (`order_lookup_`)
- `std::map` price-level insertion (`buy_book_` / `sell_book_`)

**Why this matters next:** 6F removed per-command **event-vector** churn; resting-order storage and lookup behaviour are unchanged. This is likely the **next major area** to investigate before deeper container swaps.

**Potential approaches (candidates only):**

- Pre-reserve `unordered_map` capacity when expected command/order count is known (e.g. benchmark workload size, buffered replay batch size).
- Measure whether rehashing or bucket growth occurs during benchmark workloads (Allocations / heaptrack / Instruments).
- Treat map reserve + fewer redundant lookups as a **low-risk first step** before changing core containers.

**Not guaranteed:** Reserving capacity may reduce allocations without improving wall-clock time on a given machine.

---

### 2. `std::list<Order>` node allocation

**Current role:** `std::list<Order>` at each price level gives **stable iterators** for `OrderLocation` and supports FIFO within a level. `cancel`, `reduce`, and `remove_order_at_location` depend on stored list iterators.

**Cost:** Each resting order may allocate a **separate list node**; high order churn increases heap traffic.

**Potential approaches (candidates only):**

- Compare current `std::list` against alternatives in a **controlled benchmark** (same workload, same semantics checks).
- Investigate `std::deque<Order>` **only if** iterator and reference validity can be proven safe for stored `OrderLocation` iterators across erase/reduce.
- Consider an intrusive list or custom order pool **later** — higher risk (lifetime, double-free, iterator invalidation).
- Consider `std::pmr` or an arena-backed allocator for list nodes if the **list interface stays unchanged** and invariants are unchanged.

**Do not change casually:** Affects cancellation, modification, FIFO ordering, and `order_lookup_` iterator stability. Requires strong regression tests (`validate_invariants`, FIFO/priority tests).

---

### 3. `std::map` price-level storage

**Current role:** `std::map` (with `std::greater` on the bid side) provides **ordered** price levels and simple best-price iteration in `peek_best_bid` / `peek_best_ask` and related helpers.

**Cost:** Tree-based structure — pointer chasing, per-level node allocation, cache-unfriendly traversal compared to dense layouts.

**Potential approaches (candidates only):**

- Keep `std::map` but reduce allocation cost with a pool or PMR allocator for map nodes.
- Investigate `boost::container::flat_map` or a sorted-vector style container for cache locality (measure insert/erase at price levels under workload).
- Investigate price-indexed arrays **only if** the price domain or tick range is bounded and documented.
- Investigate hash map plus explicit best-price tracking — **higher complexity** (invariant risk on cross/spread).

**Scope warning:** Larger design change than map reserve. Needs dedicated tests, repeated benchmarks, and honest “local only” reporting. Align with milestone **6G** scope in [ACTIVE_MILESTONE.md](ACTIVE_MILESTONE.md).

---

### 4. Object pooling / allocator strategy

**Context:** 6F used **caller-owned buffer reuse** for `EngineEvent` output — not a global pool. Repeated allocation in the matching path (list nodes, map nodes, hash buckets, `Order` copies including `symbol`) may eventually justify **controlled** memory reuse elsewhere.

**Potential approaches (candidates only):**

- Pool resting order nodes or price-level nodes after allocation counts are measured.
- Use monotonic or pool resources for benchmark/session lifetime allocations where lifetimes are clear.
- Introduce allocator-aware containers only after measuring **what** allocates and **how often**.
- Avoid project-wide global allocator replacement unless evidence is strong and tests are extensive.

**Risk:** Advanced milestone — increases complexity and makes lifetime bugs easier. Prefer explicit, local pools over hidden global policy.

---

### 5. Benchmark measurement overhead

**Issue:** Per-command latency measurement (`steady_clock` before/after each `process` / `process_into`) adds overhead and can widen run-to-run noise when the goal is **throughput**.

**Potential approaches (candidates only):**

- Separate a **throughput benchmark** mode from a **latency-instrumented** mode.
- Add a flag or target that disables per-command latency tracking for maximum throughput measurement.
- Keep latency tracking for p50/p95/p99 analysis; do not mix those numbers with pure throughput claims in docs or tables.

**Not guaranteed:** Throughput mode may show higher cmd/s while teaching less about tail latency.

---

### 6. Binary protocol / streaming path

**Context:** With faster engine apply (`process_into` on hot paths), relative time may shift toward **decode**, command construction, symbol handling, or buffered vs streaming replay.

**Potential approaches (candidates only):**

- Decode into reusable command or decoded-command buffers where ownership is clear.
- Reduce temporary allocations/copies during binary replay (see 6C patterns: `emplace_back`, reserved vectors).
- Avoid repeated `std::string` symbol construction on hot paths where safe and semantics-preserving.
- Compare buffered and streaming paths using **medians** over repeated runs, not a single best run.

**Boundary:** Protocol **semantics** (OBK1 v1) must not change without an explicit milestone and doc update ([BINARY_PROTOCOL.md](BINARY_PROTOCOL.md)).

---

### 7. Required process for future optimisation milestones

All future performance work in this repo should follow:

1. **Identify one** profiler-backed hotspot (CPU or allocation) — cite tool and call stack or allocation site.
2. **Make one narrow change** — no drive-by refactors or multi-milestone bundles.
3. **Preserve trading semantics** — matching rules, priority, rejection reasons, book invariants, protocol behaviour.
4. **Add or update tests** if behaviour or structural invariants could be affected.
5. Run **`./scripts/verify.sh`** from project root.
6. Run **repeated Release benchmarks** (e.g. `./scripts/benchmark_repeat.sh 5 100000 42`).
7. Compare **medians or typical** results — not the fastest single run.
8. **Document honestly** in [PERFORMANCE_BASELINE.md](PERFORMANCE_BASELINE.md) and [PROFILING_REPORT.md](PROFILING_REPORT.md), including machine-dependent noise and when before/after were not on the same commit/session.
9. **Avoid broad rewrites** without evidence — especially full container swaps without A/B profiling.

Human review and `/advance-milestone` apply per [MILESTONE_QUEUE.md](MILESTONE_QUEUE.md).

---

## Suggested investigation order (non-binding)

Updated after **9A** (2026-06-01). See [MILESTONE_9A_PERFORMANCE_PLAN.md](MILESTONE_9A_PERFORMANCE_PLAN.md).

| Priority | Candidate area | Typical risk | Notes |
|----------|----------------|--------------|--------|
| 1 | **Byte-ranked alloc profile** (9B slice 1) | Low | Instruments / `heaptrack` on engine-only apply — **required before list pool** |
| 2 | **List node allocation** in `add_order_to_side` | Medium | **9B slice 2** if slice 1 confirms dominance; not a `std::list`→`deque` swap without proof |
| 3 | `order_lookup_` / map further tuning | Low–medium | Only if slice 1 ranks above list |
| 4 | `std::map` / `std::list` container family swap | High | **Rejected** until byte-ranked evidence (6G + 9A) |
| 5 | PMR / pools (other sites) | High | After ranked profile |
| 6 | Benchmark harness throughput mode | Low | Alt 9B if alloc profile inconclusive |
| 7 | Binary decode / symbol path | Medium | Decode ~2× faster than apply on 9A baseline — lower priority than book |

This order is a planning hint only — **profiling on your machine** overrides it.

### 9A rejected for 9B (without new evidence)

- Wholesale `std::list` / `std::map` replacement
- Multithreaded SPSC for single-thread throughput
- Persistence, TCP gateway, market data publisher
- Claiming throughput wins from single `benchmark_release.sh` runs
