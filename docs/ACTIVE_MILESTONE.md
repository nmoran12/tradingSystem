# Active Milestone

**Source of truth** for `/run-active-milestone` and `/review-milestone`.

**Project root:** `cpp-low-latency-orderbook/`  
**Queue:** [MILESTONE_QUEUE.md](MILESTONE_QUEUE.md) — row **17**

**Model recommendation:** Default agent is fine for GitHub Actions and invariant tests. Use a **stronger model** for OBK1 fuzz/property work if that slice is added later.

---

## CURRENT: 8B — CI and Correctness Hardening

**Status:** Implementation **complete** — pending human `/review-milestone` before `/advance-milestone` to **8C**.

### Goal

Add project credibility through automated build/test on push/PR and stronger correctness coverage for CSV vs binary replay and the live visualisation stream path.

### Slices

#### 1. GitHub Actions CI

- Workflow on push/PR (and optionally `workflow_dispatch`).
- Steps: configure CMake, build, `ctest`, optional `cd ui/replay-visualiser && npm ci && npm run build`.
- **Shipped:** [`.github/workflows/ci.yml`](../.github/workflows/ci.yml) — Ubuntu, Debug build, two jobs (C++ tests + UI build).

#### 2. Binary vs CSV equivalence test

- Same deterministic command sequence fed through CSV parse and OBK1 binary round-trip.
- **Shipped:** `tests/test_csv_binary_engine_equivalence.cpp` (workload seed 42 + `data/sample_commands.csv`).

#### 3. Long-workload invariant tests

- **Shipped:** `tests/test_matching_engine_workload_invariants.cpp` — 2 000 commands, `validate_invariants()` after each step.

#### 4. Stable stream integration coverage

- **Shipped:** `ReplayVisualisationStreamTest.StreamsDeterministicWorkloadStepRecords` in `tests/test_replay_visualisation_stream.cpp` (in-process SSE, 30-command workload).

### Out of scope (unchanged)

- MatchingEngine / OrderBook semantic changes.
- Milestone **8C** systems work.
- Benchmark regression gates.
- OBK1 fuzz/property tests (deferred — [FUTURE_IMPROVEMENTS.md](FUTURE_IMPROVEMENTS.md)).

### Acceptance criteria

- [x] GitHub Actions workflow runs build + tests on a clean checkout (Ubuntu).
- [x] UI build step in CI (`npm ci` + `npm run build`).
- [x] Binary vs CSV equivalence test added and passing.
- [x] Long-workload invariant test added and passing.
- [x] Stream path has stable automated coverage (workload SSE test).
- [x] `./scripts/verify.sh` passes locally (**164/164**).
- [ ] Human review + `/review-milestone` before `/advance-milestone` to **8C**.

### Verification

```bash
./scripts/verify.sh
cd ui/replay-visualiser && npm run build
```

### References

- Backlog: [FUTURE_IMPROVEMENTS.md](FUTURE_IMPROVEMENTS.md)
- Binary layout: [BINARY_PROTOCOL.md](BINARY_PROTOCOL.md)
- Stream behaviour: [REPLAY_VISUALISER.md](REPLAY_VISUALISER.md)

### Intentionally deferred

- OBK1 decode fuzz / property tests.
- Stable **CLI** subprocess stream integration test (flaky pattern avoided).
- CI Release benchmark smoke / perf regression gates.
- `scripts/demo-live-replay.sh` (8A backlog).
