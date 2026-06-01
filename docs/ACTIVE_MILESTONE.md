# Active Milestone

**Source of truth** for `/run-active-milestone` and `/review-milestone`.

**Project root:** `cpp-low-latency-orderbook/`  
**Queue:** [MILESTONE_QUEUE.md](MILESTONE_QUEUE.md) — row **18**

---

## CURRENT: 8C — Next Systems Extension Decision

**Status:** Planning **complete** (revised) — [MILESTONE_8C_DECISION.md](MILESTONE_8C_DECISION.md). **No code implemented.**

**Recommended next (not CURRENT yet):** **9A — Performance Deep Dive and Hot-Path Optimisation Plan** — human adds queue row and advances before work begins.

### Goal

Choose the next major work direction. **Revised outcome:** prioritise **high-performance C++** on the existing engine/book/replay/benchmark stack; **defer** persistence, TCP, and publisher.

### Deliverable

- [x] [MILESTONE_8C_DECISION.md](MILESTONE_8C_DECISION.md) — comparison (performance vs persistence vs TCP vs publisher), recommendation (**9A perf plan**), 9A/9B scope, deferred infrastructure.

### Acceptance criteria

- [x] Comparison covers performance deep dive, persistence, TCP, publisher.
- [x] Recommendation: **9A performance planning**, not persistence.
- [x] 9A and 9B scopes documented; infrastructure explicitly deferred.
- [x] Queue not auto-advanced to 9A.
- [ ] Human review + `/advance-milestone`.

### Verification

```bash
./scripts/verify.sh   # expect 164/164; docs-only changes
```

### References

- [PERFORMANCE_ROADMAP.md](PERFORMANCE_ROADMAP.md) · [BENCHMARKING.md](BENCHMARKING.md) · [ROADMAP.md](ROADMAP.md)
