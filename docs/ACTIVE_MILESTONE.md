# Active Milestone

**Source of truth** for `/run-active-milestone` and `/review-milestone`.

**Project root:** `cpp-low-latency-orderbook/`  
**Queue:** [MILESTONE_QUEUE.md](MILESTONE_QUEUE.md) — row **17**

**Model recommendation:** Default agent is fine for GitHub Actions and invariant tests. Use a **stronger model** for OBK1 fuzz/property work if that slice is added in-session.

---

## CURRENT: 8B — CI and Correctness Hardening

**Status:** `READY` — not started. Do not implement from this advance step; use `/run-active-milestone` in a **new** session.

### Goal

Add project credibility through automated build/test on push/PR and stronger correctness coverage for CSV vs binary replay and the live visualisation stream path.

### Slices

#### 1. GitHub Actions CI

- Workflow on push/PR (and optionally `workflow_dispatch`).
- Steps: configure CMake, build, `./scripts/verify.sh` or equivalent `ctest`, optional `cd ui/replay-visualiser && npm ci && npm run build`.
- **Not yet in repo** — this is the primary 8B deliverable.

#### 2. Binary vs CSV equivalence test

- Same deterministic command sequence fed through `--engine` (CSV) and `--binary-engine` (`.obk`).
- Assert equivalent final book state / trade counts (define comparison helpers; no silent semantic drift).
- Document any intentional differences if discovered.

#### 3. Long-workload invariant tests

- After large `WorkloadGenerator` or SPSC pipeline runs, call `OrderBook::validate_invariants()` (or existing introspection).
- Seed-controlled workloads; keep runtime reasonable for CI.

#### 4. Stable stream integration coverage

- Reliable test for `--stream-visualisation` / SSE path.
- **Do not** reintroduce the flaky CLI subprocess pattern removed in 7B; prefer in-process server + client (see existing `tests/test_replay_visualisation_stream.cpp`).

### Out of scope

- Changing `MatchingEngine` or `OrderBook` matching semantics.
- Milestone **8C** systems work (TCP gateway, persistence, market data publisher).
- Benchmark throughput regression gates without an explicit, reviewed design.
- OBK1 fuzz/property tests — **optional** stretch; backlog item in [FUTURE_IMPROVEMENTS.md](FUTURE_IMPROVEMENTS.md), not required to close 8B if time-boxed.
- Demo script, README screenshots, or further 8A doc polish.

### Constraints

- Preserve architecture boundaries ([ARCHITECTURE.md](ARCHITECTURE.md), [DEVELOPMENT_RULES.md](DEVELOPMENT_RULES.md)).
- All existing tests must keep passing; add tests for new behaviour.
- Do not commit `profiling/` or machine-local trace artifacts.
- CI must not imply features that do not exist (no deploy, no cloud product).

### Acceptance criteria

- [ ] GitHub Actions workflow runs build + **160** tests on a clean checkout (or documents any platform matrix limitation).
- [ ] Optional UI build step passes in CI (or is clearly documented as optional/skipped with reason).
- [ ] Binary vs CSV equivalence test added and passing.
- [ ] At least one long-workload invariant test added and passing.
- [ ] Stream path has stable automated coverage (extend existing tests or add focused test).
- [ ] `./scripts/verify.sh` passes locally after changes.
- [ ] Human review + `/review-milestone` before `/advance-milestone` to **8C**.

### Verification

```bash
./scripts/verify.sh
cd ui/replay-visualiser && npm run build   # if UI included in CI scope
```

### References

- Backlog: [FUTURE_IMPROVEMENTS.md](FUTURE_IMPROVEMENTS.md) §CI and testing
- Binary layout: [BINARY_PROTOCOL.md](BINARY_PROTOCOL.md)
- Stream behaviour: [REPLAY_VISUALISER.md](REPLAY_VISUALISER.md)
- Prior milestone (done): [DEMO_GUIDE.md](DEMO_GUIDE.md) — `f4116b3`

### Previous milestone (8A — done)

| Check | Result |
|-------|--------|
| Docs-only delivery | `f4116b3` |
| `./scripts/verify.sh` | **160/160** at review |
