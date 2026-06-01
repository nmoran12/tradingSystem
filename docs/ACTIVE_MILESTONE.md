# Active Milestone

**Source of truth** for `/run-active-milestone` and `/review-milestone`.

**Project root:** `cpp-low-latency-orderbook/`  
**Queue position:** See [MILESTONE_QUEUE.md](MILESTONE_QUEUE.md) — **6H is CURRENT**

---

## Milestone 6H — Optional SPSC Queue

| Field | Value |
|-------|--------|
| **Status** | READY (workflow) |
| **Parent** | Milestone 6 — Performance and optimisation groundwork |
| **Test baseline** | 129 tests after 6G order lookup reservation pass |

### Model recommendation

Use a **strong performance-focused coding model**. SPSC queues involve memory ordering, capacity edge cases, and later pipeline equivalence with `MatchingEngine` — correctness and documented semantics first.

---

### Goal

Introduce a **single-producer / single-consumer (SPSC)** ring buffer pipeline that **wraps** `MatchingEngine` command ingest (and optionally event egress later) to practice low-latency queue patterns **without** making the engine multi-threaded internally or changing matching semantics.

Direct `process` / `process_into` on the engine must remain the semantic baseline; any pipeline path must prove **equivalence** on fixed workloads before claiming performance benefit.

### First slice (planned)

1. **`include/concurrency/SpscRingBuffer.hpp`** — fixed-capacity template with `try_push` / `try_pop`, acquire/release atomics, power-of-two capacity (mask indexing) where practical; document full/empty behaviour.
2. **`tests/test_spsc_ring_buffer.cpp`** — single-threaded FIFO, full-buffer reject, empty pop fail, wrap-around stress (no `MatchingEngine` yet).
3. **Do not** in this first slice:
   - integrate with `MatchingEngine` or change engine/book code
   - add MPMC queues, mutexes in the hot path, or networking
   - add TCP, persistence, or UI work

**Slice 2 (done in tree):** `pipeline::SpscCommandPipeline` — deterministic single-threaded enqueue/drain around `process_into`; equivalence tests vs direct processing (no throughput claims).

**Slice 3 (done in tree):** `WorkloadGenerator` equivalence tests (100 and 1 000 commands, seed 42; large-queue `run_sequence` and small-queue interleaved enqueue/drain). Correctness only — no throughput claims.

Later slices: optional output ring, pipeline benchmark vs direct loop — see [MILESTONE_6_PLAN.md](MILESTONE_6_PLAN.md).

### Scope

- SPSC ring buffer API and correctness tests
- Cache-line awareness on head/tail where appropriate (document layout)
- Optional two-thread tests **only** if explicitly labeled and deterministic
- Keep **replay path** and **engine path** behaviour unchanged until an equivalence-tested pipeline adapter exists
- Document design in header comments + short `docs/` note when behaviour is non-obvious

### Out of scope

- Changing `OrderBook` / `MatchingEngine` matching or storage semantics
- MPMC queues, lock-free engine internals, or multithreaded matching
- Networking, TCP gateway, replication, persistence
- Binary protocol semantic changes
- Further order-book container optimisation (completed under **6G**)
- Benchmark throughput claims before pipeline equivalence is proven

### Acceptance criteria

- [ ] `./scripts/verify.sh` passes (new buffer tests included)
- [ ] Ring buffer API documented (full/empty, capacity, memory ordering intent)
- [ ] Single-threaded buffer tests cover FIFO, boundary, and wrap-around
- [ ] No engine integration or matching/book changes in the first slice unless acceptance criteria are explicitly expanded by human review
- [ ] No later-milestone work (7A+) in the same change set

### Required verification

From project root:

```bash
./scripts/verify.sh
```

Pipeline benchmarks (later slices only):

```bash
./scripts/benchmark_repeat.sh 5 100000 42
```

### Key docs

- [MILESTONE_6_PLAN.md](MILESTONE_6_PLAN.md)
- [ARCHITECTURE.md](ARCHITECTURE.md) — future `concurrency/` boundary
- [DEVELOPMENT_RULES.md](DEVELOPMENT_RULES.md)
- [PROFILING_REPORT.md](PROFILING_REPORT.md)

### Human review before advance

After `/run-active-milestone` completes, run `/review-milestone`, then human approval before `/advance-milestone`.
