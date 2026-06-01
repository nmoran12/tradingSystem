# Active Milestone

**Source of truth** for `/run-active-milestone` and `/review-milestone`.

**Project root:** `cpp-low-latency-orderbook/`  
**Queue position:** See [MILESTONE_QUEUE.md](MILESTONE_QUEUE.md) — **no milestone CURRENT**

---

## Queue complete (through 7C)

The ordered backlog in [MILESTONE_QUEUE.md](MILESTONE_QUEUE.md) is **finished** through **7C — UI Metrics and Benchmark Overlay** (commit `7423903`).

Milestone 7 visualisation track summary:

| ID | Name | Status |
|----|------|--------|
| 7A | Replay Visualiser UI (file export + UI) | Done |
| 7B | Live Replay Streaming Interface | Done |
| 7C | Run summary metrics overlay | Done |

Do **not** start new implementation from this file until a human adds the next queue row and marks it **CURRENT**.

---

## Choosing the next milestone

Candidates live in [ROADMAP.md](ROADMAP.md) but are **not** automatically queued. Examples:

| ROADMAP item | Summary |
|--------------|---------|
| **Milestone 7 TCP** | TCP order gateway; wire format → `OrderCommand` → `EngineEvent` |
| **Milestone 8** | Market data publisher from engine events |
| **Milestone 9** | Persistence and deterministic replay |

**Workflow:** agree on the next ID → add a row to [MILESTONE_QUEUE.md](MILESTONE_QUEUE.md) as **CURRENT** → replace this file’s body with that milestone’s goal/slices → run `/run-active-milestone` in a new session.

---

## Last completed: 7C (reference)

| Field | Value |
|-------|--------|
| **Deliverable** | Run summary panel — client-side metrics from `schemaVersion: 1` steps |
| **Verification** | `npm run build` (UI); `./scripts/verify.sh` (160 tests) |
| **Out of scope met** | No matching-core, benchmark, or export-schema changes |

Key docs: [REPLAY_VISUALISER.md](REPLAY_VISUALISER.md), [BENCHMARKING.md](BENCHMARKING.md).
