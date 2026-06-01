# Milestone 8C — Next milestone direction (revised)

**Status:** Planning artifact (8C deliverable). **Not implemented** — no new persistence, TCP, publisher, or hot-path code in this milestone.

**Decision date:** 2026-06-01 (revised after initial 8C draft).
**Baseline:** post-8B, CI green, **164** tests, Release benchmarks and profiling docs exist ([BENCHMARKING.md](BENCHMARKING.md), [PERFORMANCE_BASELINE.md](PERFORMANCE_BASELINE.md), [PROFILING_REPORT.md](PROFILING_REPORT.md)).

**Related:** [ROADMAP.md](ROADMAP.md) · [ACTIVE_MILESTONE.md](ACTIVE_MILESTONE.md) · [PERFORMANCE_ROADMAP.md](PERFORMANCE_ROADMAP.md) · [FUTURE_IMPROVEMENTS.md](FUTURE_IMPROVEMENTS.md)

---

## 1. Current project direction

### Focus: high-performance C++ matching engine

The project should remain a **credible low-latency C++ order-book and matching-engine** codebase first. Packaging (8A), CI/correctness (8B), and optional visualisation (7A–7C) support that story but do not replace a **measured, honest performance narrative** on the core hot path.

| Principle | Meaning |
|-----------|---------|
| **Core before infrastructure** | Improve and evidence the engine/book/replay/benchmark loop before adding journals, gateways, or pub/sub products. |
| **Profiler-driven changes only** | No container swaps or “optimisations” without dated Release runs and Instruments (or equivalent) on this machine. |
| **Correctness preserved** | 164 tests + equivalence/invariant coverage from 8B must stay green. |
| **No hype** | Report medians/repeated runs; no production SLA claims. |

### Why defer new infrastructure for now

Infrastructure milestones (persistence, TCP, publisher) are **valuable later** but **expand scope** without directly strengthening the current matching-engine performance story. The next queue row should **measure, plan, and then narrowly optimise** what already exists.

---

## 2. Candidate comparison

Summary matrix (four options):

| Dimension | **D. Performance deep dive (recommended)** | B. Persistence / replay log | A. TCP order gateway | C. Market data publisher |
|-----------|---------------------------------------------|------------------------------|----------------------|---------------------------|
| **Fits current priority** | **Yes** — core C++ perf | No — new subsystem | No — networking scope | No — overlaps viz |
| **Engineering value** | **High** (methodology + targeted hot-path wins) | High (durability) | High (I/O boundary) | Medium |
| **Resume signal** | **Strong** (“profiled and optimised matching hot path with evidence”) | Strong (WAL/replay) | Strong (gateway) | Moderate |
| **Difficulty** | Medium (plan) + Medium (one optimisation) | Large | Large | Medium |
| **Scope creep risk** | Low–medium if sliced | Medium | **High** | Medium |
| **Builds on 8B** | **Yes** (stable CI + correctness baseline) | Yes (equivalence tests) | Partial | Weak |
| **Overlap with shipped work** | **Extends 6A–6H track** | Low | Low | **High** (7B SSE, NDJSON) |

---

### D. Performance deep dive and hot-path optimisation (recommended)

**What it would add**

- A **structured performance programme** on the existing code: Release benchmarks, profiling, hotspot report, separation of engine vs parse/replay/viz cost, dated baseline table, and a **short, evidence-backed optimisation plan** (9A planning → 9B implementation).
- At most **one or two** low-risk optimisations in 9B, chosen only after measurement (examples: allocation in order storage, `order_lookup_` / reserve strategy, level traversal, `EngineEvent` vector churn, binary replay hot path — **not pre-committed**).

**Why it matters**

- Recruiters and reviewers already see matching + binary + benchmarks; the gap is a **clear, reproducible “what we measured and what we improved”** chapter.
- Aligns with project history (6E–6G rejected or accepted changes based on profiler output).

**Deferred to 9B:** Any code change. **9A is planning and measurement only.**

---

### B. Persistence / replay log — **deferred**

**Value:** Append-only command journal, deterministic recovery replay, crash simulation — excellent systems story.

**Why defer now:** Adds a **new persistence subsystem** (`include/persistence/`, journal format, CLI flags) before the core performance narrative is refreshed. Does not shorten the matching hot path by itself. Revisit **after 9A/9B** when the engine story is stronger.

---

### A. TCP order gateway — **deferred**

**Value:** OBK1-framed TCP → `MatchingEngine` → streamed `EngineEvent`s; gateway module stays out of the engine.

**Why defer now:** Expands into **networking** (sessions, framing, backpressure, flaky tests) while performance work on the existing single-process path is unfinished. Natural **after** measured engine optimisations (or after a minimal journal if audit trail is needed later).

---

### C. Market data publisher — **deferred**

**Value:** In-process subscribers for trades/BBO from `EngineEvent`s.

**Why defer now:** **Overlaps** 7A NDJSON export and 7B localhost SSE visualisation. Weaker incremental signal than a profiler-backed engine improvement.

---

## 3. Revised recommendation

### Primary choice: **9A — Performance Deep Dive and Hot-Path Optimisation Plan**

| Decision | Choice |
|----------|--------|
| **Next recommended milestone** | **9A** — planning, profiling, baselines, optimisation **plan** (no mandatory code in 9A) |
| **Following milestone** | **9B** — implement **at most one** measured optimisation from 9A if evidence supports it |
| **Persistence** | **Deferred** (was initial 8C draft; not queued) |
| **TCP gateway** | **Deferred** |
| **Market data publisher** | **Deferred** |

### Why performance before persistence (revision note)

An earlier 8C draft recommended persistence first for determinism and CI-friendly tests. That remains valid **for a later phase**, but the **current owner priority** is **high-performance C++ on the existing engine**, not new infrastructure. Persistence should return when the hot path has a **dated baseline and a documented optimisation outcome** (or a deliberate decision that the core is “good enough”).

---

## 4. Proposed milestone **9A** — Performance deep dive (planning)

> **Not CURRENT** until human adds queue row 19 and `/advance-milestone`. **No implementation in 8C.**

### Goal

Produce an evidence-based picture of where time goes today and a **scoped, low-risk optimisation plan** for the matching engine, order book, and replay/benchmark paths.

### Slices (9A — docs + measurement, minimal code)

| # | Slice | Deliverable |
|---|--------|-------------|
| 1 | Clean Release baselines | Re-run `./scripts/benchmark_release.sh` (and repeat script if used); record **dated** table in [PERFORMANCE_BASELINE.md](PERFORMANCE_BASELINE.md) or `docs/PERF_9A_BASELINE.md` |
| 2 | Re-profile engine | Instruments / Time Profiler on `matching_engine_benchmark` and/or binary engine apply path ([PROFILING_REPORT.md](PROFILING_REPORT.md) updated) |
| 3 | Hotspot ranking | Top N functions with **% time** and call-path notes (engine-only vs I/O) |
| 4 | Cost separation | Table: engine loop vs CSV/binary parse vs viz export/stream vs SPSC pipeline (off hot path by default) |
| 5 | Data-structure review | Document current `OrderBook` (`std::map` + `std::list` + `unordered_map`), allocation sites, `process_into` / event vector behaviour — **no rewrite without evidence** |
| 6 | Optimisation candidates | 2–4 **candidates** ranked by impact/risk; pick **0–2** for 9B with go/no-go criteria |
| 7 | 9B plan doc | Short plan: target, expected metric, tests, rollback if benchmarks regress |

### 9A acceptance criteria

- [ ] Dated Release baseline table committed (machine-labelled, not portable claims).
- [ ] Profiler report section for current `HEAD` (post-8B).
- [ ] Written hotspot list with engine vs non-engine split.
- [ ] Explicit **9B** recommendation(s) tied to profiler rows — or explicit “no change” if within noise.
- [ ] No matching-semantics changes in 9A.
- [ ] `./scripts/verify.sh` still **164/164** if any doc-only edits.

### 9A constraints

- Planning and measurement first; **no required engine/book code change in 9A**.
- No new infrastructure (journal, TCP, publisher).
- No benchmark regression gates in CI.
- Do not commit `profiling/` trace bundles.

---

## 5. Proposed milestone **9B** — One measured optimisation (implementation)

> **Queued only after 9A is reviewed.** Implement **zero or one** primary change (two only if tiny and separately evidenced).

### Goal

If and only if 9A identifies a justified target, implement it and prove **no correctness regression** and **documented** benchmark delta.

### Example targets (not pre-selected)

- Reduce allocations in order storage / level nodes.
- Tune `reserve_active_orders` / hash load for workload shape.
- Reduce per-command `EngineEvent` vector churn (caller buffer patterns).
- Binary replay read/decode/apply path (if profiler shows dominance).
- Order-book level traversal micro-optimisations (if safe and measured).

### 9B acceptance criteria (sketch)

- [ ] Change linked to 9A profiler line item.
- [ ] `./scripts/verify.sh` **164/164**; new tests only if behaviour surface changes (prefer none).
- [ ] Before/after benchmark table (same seed/count, Release, repeated runs).
- [ ] If no improvement or regression within noise: document **no-merge** and stop.

### 9B out of scope

- `std::list` / `std::map` wholesale replacement without profiler proof (6G lesson).
- Multithreaded matching engine.
- Persistence, TCP, publisher, UI, CI benchmark gates.

---

## 6. Proposed order after 8C

| Order | ID | Name | Type |
|-------|-----|------|------|
| 1 | **9A** | Performance deep dive and hot-path optimisation **plan** | **Recommended next** (planning) |
| 2 | **9B** | One measured hot-path optimisation | Implementation (conditional) |
| 3 | **10A** | Persistence / command journal (revisit) | Infrastructure (deferred) |
| 4 | **10B** | TCP order gateway (localhost, OBK1) | Infrastructure (deferred) |
| 5 | **10C** | Market data publisher | Infrastructure (deferred) |

**8C** remains **CURRENT** until human `/advance-milestone` (8C Done → queue **9A**).

---

## 7. Scope boundaries (reconfirmed)

This project must **not** become:

| Avoid | Note |
|-------|------|
| Full exchange / brokerage | Educational simulator |
| Trading dashboard product | UI is demo-only |
| Auth / accounts / cloud | Out of scope |
| Database-backed journal in 9A/9B | SQL/cloud deferred |
| Benchmark claim machine | Dated local evidence only |
| Speculative container rewrite | Requires profiler proof (6G) |

**Untracked:** `profiling/` trace directories — do not commit.

---

## 8. Decision summary

| Question | Answer |
|----------|--------|
| **Recommended next milestone** | **9A — Performance Deep Dive and Hot-Path Optimisation Plan** |
| **Persistence** | **Deferred** — valuable, not current priority |
| **TCP gateway** | **Deferred** — networking scope |
| **Market data publisher** | **Deferred** — overlaps visualiser |
| **8C code changes** | **None** — this document and roadmap updates only |

**Human next steps:** Review this revision → `/review-milestone` → `/advance-milestone` (8C Done, add row **19 | 9A | …** as CURRENT) → `/run-active-milestone` for 9A measurement/planning in a new session.
