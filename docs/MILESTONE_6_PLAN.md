# Milestone 6 Plan: Ring Buffer Event Pipeline

**Status:** Not started (planning only)  
**Prerequisite:** Milestone 5 recommended but not strictly required; Milestones 1–4 complete

## Goal

Introduce a **single-producer / single-consumer (SPSC)** ring buffer pipeline around `MatchingEngine` to practice low-latency queue patterns **without** making the engine itself multi-threaded or changing matching correctness.

## Principles

- **Wrap** the engine; do not rewrite `MatchingEngine` internals.
- **SPSC only** (no MPMC in M6).
- **No mutex** in ring buffer hot path; use atomics.
- **Cache-line padding** on head/tail where appropriate.
- **Deterministic** single-threaded tests for engine remain unchanged.
- Pipeline tests may use two threads only where explicitly testing concurrency.
- **No networking** in Milestone 6.

---

## Milestone 6A: SPSC ring buffer implementation

**Goal:** Generic fixed-capacity ring buffer template.

**New files:**

- `include/concurrency/SpscRingBuffer.hpp` (header-only or .cpp if needed)

**Behaviour:**

- Template over `T` (start with `OrderCommand` or a small `CommandSlot` struct)
- Capacity power-of-two (optional) for mask indexing
- `try_push(T)` / `try_pop(T&)` or `push` blocking vs `try_*` non-blocking—document choice (prefer `try_*` for benchmark)
- `size()`, `empty()`, `full()` queries
- Head/tail atomics with acquire/release ordering

**Edge cases:**

- Push when full → false
- Pop when empty → false
- Capacity 1 and N boundary

**Tests:** deferred to 6B

**Acceptance:**

- Compiles; documented API in header comment

**Out of scope:** MatchingEngine integration

---

## Milestone 6B: Ring buffer tests

**Goal:** Correctness tests without engine.

**New files:**

- `tests/test_spsc_ring_buffer.cpp`

**Tests:**

- Push/pop FIFO order
- Fill buffer then reject push
- Empty pop fails
- Wrap-around indices after many operations
- Single-threaded stress: push N pop N

**Optional (explicitly labeled concurrent tests):**

- Producer thread + consumer thread deterministic pattern (fixed seed of operations)

**Acceptance:**

- All buffer tests pass
- All 56 existing tests still pass

**Out of scope:** Engine in loop

---

## Milestone 6C: Input pipeline simulation

**Goal:** Producer feeds `OrderCommand`s into ring buffer from `WorkloadGenerator` or preloaded vector.

**New files:**

- `include/pipeline/CommandPipeline.hpp` (optional)
- `src/pipeline/CommandPipeline.cpp` (optional)

**Behaviour:**

- Producer stage: iterate commands → `try_push`
- Handle full buffer (spin, drop, or back-pressure policy—document; prefer return error in benchmark)
- Deterministic when single-threaded simulated producer fills queue before consumer starts

**Edge cases:**

- Full buffer during benchmark
- Zero-capacity invalid config

**Tests:**

- Single-threaded: preload queue, drain, order preserved

**Acceptance:**

- Order preservation test passes

**Out of scope:** Output queue, threads

---

## Milestone 6D: Engine consumer loop

**Goal:** Consumer drains buffer and calls `MatchingEngine::process`.

**Likely changes:**

- `CommandPipeline` or `benchmarks/benchmark_ring_buffer_pipeline.cpp` skeleton

**Behaviour:**

```text
while (pop(cmd)) {
  events = engine.process(cmd);
  // discard or count events
}
```

- Same results as direct loop for same command sequence (equivalence test)

**Edge cases:**

- Partial drain
- Empty buffer at end

**Tests:**

- Equivalence: direct `process` vs pipeline drain → same `active_order_count` and `total_resting_quantity` (and optionally trade count)

**Acceptance:**

- Equivalence test passes for fixed small workload

**Out of scope:** `EngineEvent` output queue

---

## Milestone 6E: Output event queue (optional but planned)

**Goal:** Decouple event handling from consumer hot path.

**New files:**

- `include/concurrency/SpscRingBuffer.hpp` — reuse for `EngineEvent` or lightweight `EventSummary`
- Or second ring `SpscRingBuffer<EngineEvent>` (may be large—consider `EventSummary { type, order_id, trade_qty }` for benchmark)

**Behaviour:**

- Consumer pushes summary events to output ring
- Separate drain for logging/stats (single-threaded drain OK in M6)

**Edge cases:**

- Output ring full (drop vs block—document)

**Tests:**

- Event count matches direct processing

**Acceptance:**

- Trade count matches baseline for test workload

**Out of scope:** Real async I/O thread

---

## Milestone 6F: Pipeline benchmark

**Goal:** Compare throughput: direct vs pipeline.

**New files:**

- `benchmarks/benchmark_ring_buffer_pipeline.cpp`

**Metrics:**

- Same as [BENCHMARKING.md](BENCHMARKING.md) where applicable
- Note overhead of push/pop vs direct `process`

**Behaviour:**

- Fixed seed, same command count as CLI arg
- Report both modes side-by-side

**Acceptance:**

- Benchmark builds and runs
- Numbers documented as machine-dependent

**Out of scope:** Claiming pipeline is faster (may be slower until optimised)

---

## Milestone 6G: README and docs update

**Goal:** Document pipeline architecture.

**Files:**

- `docs/ARCHITECTURE.md` — add pipeline diagram
- `docs/ROADMAP.md` — mark M6 complete
- `README.md` — link pipeline benchmark

**Acceptance:**

- Docs match code; boundaries clear (pipeline wraps engine)

---

## Suggested file tree (final)

```
include/concurrency/
  SpscRingBuffer.hpp
include/pipeline/          (optional)
  CommandPipeline.hpp
src/pipeline/
  CommandPipeline.cpp
tests/
  test_spsc_ring_buffer.cpp
  test_command_pipeline.cpp   (equivalence)
benchmarks/
  benchmark_ring_buffer_pipeline.cpp
```

## Design notes

### Cache-line padding

```cpp
alignas(64) std::atomic<size_t> head_;
alignas(64) std::atomic<size_t> tail_;
```

Separate producer/consumer atomics to reduce false sharing.

### Why not MPMC

MPMC adds complexity and non-determinism in tests. Milestone 6 is learning SPSC first.

### MatchingEngine unchanged

All `MatchingEngineTest` cases run without pipeline. Pipeline is integration-layer only.

## Global acceptance criteria (Milestone 6 complete)

- [ ] SPSC push/pop correct under tests
- [ ] Full/empty behaviour tested
- [ ] FIFO order preserved
- [ ] Pipeline processes commands in deterministic order (single-threaded equivalence)
- [ ] All **56** existing tests pass **unchanged**
- [ ] New tests pass (target: 10+ buffer/pipeline tests)
- [ ] Pipeline benchmark executable runs
- [ ] No mutex in ring buffer hot path
- [ ] No changes to matching logic inside `MatchingEngine.cpp` unless bugfix (document separately)

## Explicitly out of scope (entire milestone)

- MPMC / MP-SC queues
- Lock-free `MatchingEngine` internal state
- TCP gateway (Milestone 7)
- Custom memory pools
- Replacing `std::vector<EngineEvent>` return type with callback (future refactor)
- Production thread pinning / NUMA

## Risk register

| Risk | Mitigation |
|------|------------|
| Data races in tests | Keep engine tests single-threaded; isolate threaded tests |
| `EngineEvent` too large for ring | Use summary struct in ring |
| Performance regression confusion | Report direct vs pipeline side-by-side |
| Scope creep into TCP | Gate M7 separately |

## Suggested implementation order

6A → 6B → 6C → 6D → (6E if time) → 6F → 6G

## Dependency on Milestone 5

Binary-encoded commands can later be pushed into the ring as `std::vector<uint8_t>` or decoded `OrderCommand` slots. Milestone 6 can proceed with `OrderCommand` payloads first; integrate binary in a follow-up sub-task if M5 completes first.
