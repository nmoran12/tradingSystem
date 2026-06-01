# Development Rules

Strict guardrails for `cpp-low-latency-orderbook`. All contributors should follow these rules.

## Core rules

1. **Do not rewrite working systems** without an explicit reason tied to the current milestone.
2. **Keep existing tests passing** after every change (currently 123 GoogleTest cases).
3. **Add tests for every new behaviour**—happy path and important edge cases.
4. **Prefer small, milestone-based changes** over large refactors.
5. **Keep public APIs stable** where possible; document breaking changes.
6. **Do not mix unrelated milestones** in one PR or agent session (e.g. binary protocol + ring buffer).
7. **Keep benchmark and demo code separate** from core matching/book logic.
8. **Favour correctness before optimisation.**
9. **Document design tradeoffs** in code comments only when non-obvious; prefer `docs/` for milestone decisions.

## Architecture rules

| Component | Responsibility | Must not |
|-----------|----------------|----------|
| `OrderBook` | Storage, FIFO, lookup, replay `apply_event` | Matching policy for engine commands |
| `MatchingEngine` | Match, market, modify, emit `EngineEvent` | Parse files, network I/O |
| `MarketDataParser` | `MarketEvent` CSV | Engine matching |
| `OrderCommandParser` | `OrderCommand` CSV | Engine matching |
| `EngineEventPrinter` | Format output | Trading logic |
| `LatencyTracker` | Timing stats | Business logic |
| `WorkloadGenerator` / benchmark | Synthetic load, metrics | Change match results |
| `main.cpp` | CLI orchestration | Deep business logic |

- **Replay path and engine path stay separate** unless an explicit adapter is added and documented.
- **Do not expose** internal book containers (`buy_book_`, `sell_book_`, etc.) outside `OrderBook`.

## C++ rules

- **C++20** minimum; match existing style (`enum class`, `std::optional`, snake_case methods).
- **Clear ownership:** no raw owning pointers; RAII.
- **Avoid unnecessary heap allocation** in hot paths when reasonable—but do not introduce custom allocators without benchmark justification.
- **No premature custom allocators** or lock-free structures unless the milestone requires them.
- **Avoid global mutable state** except where already established (e.g. none in engine/book).
- **Minimal includes** in headers; forward-declare when practical.
- **Small, testable functions**; parsers and engine logic should be unit-testable without CLI.
- **Deterministic behaviour** for matching and synthetic workloads (fixed seeds in tests/benchmarks).

## Testing rules

- Every milestone **must add tests** before or with implementation.
- Include **edge cases:** empty book, partial fill, duplicate ID, unknown cancel, malformed parse rows.
- **Keep regression tests**—never delete tests to make implementation pass.
- Use **descriptive test names** (`MatchingEngineTest`, `MarketOrderTest`, etc.).
- Run full suite after changes:

```bash
cd cpp-low-latency-orderbook
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

## Performance rules

- **Do not optimise before correctness** and tests are green.
- Use **benchmarks to justify** optimisation (compare before/after on same machine, Release build).
- **Fixed seeds** for `WorkloadGenerator` in tests and documented benchmark runs.
- **Document benchmark results** as machine-dependent relative comparisons—not production SLA claims.
- Prefer **Release builds** for benchmark numbers: `-DCMAKE_BUILD_TYPE=Release`.

## Documentation and milestone discipline

- **Read docs first** for major work: `PROJECT_OVERVIEW.md`, `ARCHITECTURE.md`, `ROADMAP.md`, `DEVELOPMENT_RULES.md`, milestone plan if applicable.
- **Produce a plan** before coding a new milestone (files, tests, risks, acceptance).
- **Do not invent** directories or components that do not exist; follow `include/` layout.
- **Do not rename files** unless the milestone requires it.
- **Do not silently change behaviour** of replay, matching, or parsers—call it out in summary.
- **Summarise changes** after completion: files touched, tests added, commands run.
- **Do not implement** networking, ring buffers, custom allocators, persistence, or replication unless explicitly requested for that milestone.

## Project directory reminder

CMake project root is:

```text
cpp-low-latency-orderbook/
```

Not the parent `tradingSystem/` folder. Always run `cmake -S . -B build` from inside `cpp-low-latency-orderbook`.

## Milestone completion checklist

- [ ] Behaviour matches acceptance criteria in `ROADMAP.md` / milestone plan
- [ ] New tests added and named clearly
- [ ] All tests pass (`./scripts/verify.sh` — currently **160**)
- [ ] Replay CLI still works (`--replay`)
- [ ] Engine CLI still works (`--engine`) if engine touched
- [ ] Benchmark still runs if perf-related code touched
- [ ] `docs/` and `README.md` updated when user-facing behaviour changes
- [ ] No unrelated drive-by refactors
