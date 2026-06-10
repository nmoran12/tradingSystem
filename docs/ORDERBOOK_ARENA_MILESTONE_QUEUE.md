# OrderBook Arena Milestone Queue

This queue covers the first implementation path after planning. Complete one
milestone at a time and keep the existing matching behaviour stable.

## Queue

| Order | ID | Milestone | Status |
|------:|----|-----------|--------|
| 1 | A1 | Website/Product Skeleton and Challenge Browsing | Ready |
| 2 | A2 | Challenge Definition Schema | Planned |
| 3 | A3 | Local Deterministic Challenge Runner | Planned |
| 4 | A4 | Built-in Baseline Strategies | Planned |
| 5 | A5 | Scoring and Result JSON | Planned |
| 6 | A6 | Replay Export and Website Replay Viewer | Planned |
| 7 | A7 | Python Strategy API | Planned |
| 8 | A8 | Optional Leaderboard/Result Submission Spike | Stretch |

## A1: Website/Product Skeleton and Challenge Browsing

**Goal:** establish the website as the place to discover and understand
challenges.

**Deliverables:** website shell, challenge catalogue mock data, challenge detail
page, starter instructions, sample result section, and replay-viewer link.

**Acceptance criteria:**

- At least two sample challenge cards can be browsed.
- One detail page shows prompt, rules, constraints, scoring summary, and local
  run instructions.
- The site clearly says strategies execute locally.
- Existing replay visualiser behaviour remains available.

**Tests/checks:** frontend build, basic component/parser tests, responsive manual
check, and link check.

**Why it matters:** defines the user experience before backend contracts harden.

## A2: Challenge Definition Schema

**Goal:** define one versioned format shared by the website and local runner.

**Deliverables:** schema, validator, one complete challenge, starter metadata,
and clear validation errors.

**Acceptance criteria:**

- Presentation, scenario, risk, seed, and scoring fields can be loaded.
- Unknown versions and invalid ranges fail clearly.
- The format contains no executable code.
- Website data and runner data come from the same challenge definition.

**Tests/checks:** valid fixture, required-field failures, invalid ranges, unknown
version, and stable canonical serialization if used.

**Why it matters:** prevents the website and judge from describing different
challenges.

## A3: Local Deterministic Challenge Runner

**Goal:** run one seeded single-instrument challenge through the existing engine.

**Deliverables:** CLI command, simulated clock, scenario generator, episode
loop, portfolio/risk state, and output directory.

**Acceptance criteria:**

- One command runs a challenge and public seed set locally.
- The same inputs produce the same canonical events and state.
- Different seeds produce different valid episodes.
- Existing engine tests pass and book invariants hold.

**Tests/checks:** golden scenario, repeated-run equivalence, different-seed
test, invariant stress test, CLI smoke test, and existing C++ tests.

**Why it matters:** creates the deterministic judge without server execution.

## A4: Built-in Baseline Strategies

**Goal:** define the strategy contract with controlled in-process examples.

**Deliverables:** observation/action types, no-op baseline, and one active
baseline.

**Acceptance criteria:**

- Strategies cannot mutate engine state directly.
- Invalid actions are handled consistently.
- Baseline results are deterministic and suitable for challenge pages.

**Tests/checks:** observation fixtures, action validation, baseline golden
results, and repeated-run equivalence.

**Why it matters:** proves the strategy interface and gives users reference
results.

## A5: Scoring and Result JSON

**Goal:** produce an explainable local result.

**Deliverables:** portfolio accounting, score components, penalties, versioned
result JSON, and batch seed summary.

**Acceptance criteria:**

- Result JSON records challenge, engine, strategy, protocol, and seed versions.
- Raw metrics explain the final score.
- Risk breaches and failures have explicit results.
- Identical completed episodes receive identical scores.

**Tests/checks:** hand-calculated scores, no-trade run, profitable and losing
runs, risk breach, failure result, and schema tests.

**Why it matters:** turns local simulation into a challenge outcome that can be
shown or compared.

## A6: Replay Export and Website Replay Viewer

**Goal:** inspect locally generated runs in the website.

**Deliverables:** versioned Arena event log, replay export, browser file loader,
and views for strategy actions, fills, inventory, and score components.

**Acceptance criteria:**

- A CLI-generated replay opens without uploading to a server.
- Existing replay samples remain supported.
- Displayed result metadata matches the result JSON.
- Export is disabled outside explicit Arena runs or replay modes.

**Tests/checks:** golden replay, replay/result equivalence, malformed file
handling, frontend build, and end-to-end manual replay.

**Why it matters:** makes a score understandable and connects the local runner
to the website.

## A7: Python Strategy API

**Goal:** let users solve challenges with a local Python strategy.

**Deliverables:** versioned line-oriented protocol, starter strategy, local
process adapter, timeout, and failure handling.

**Acceptance criteria:**

- Python receives observations and returns actions.
- A fixed strategy and seed set produce repeatable results.
- Timeout, crash, malformed output, and invalid actions fail cleanly.
- Documentation states that local process handling is not a secure sandbox.

**Tests/checks:** successful strategy, built-in/Python equivalence where
applicable, timeout, process failure, malformed response, and invalid action.

**Why it matters:** provides the first practical user coding workflow.

## A8: Optional Leaderboard/Result Submission Spike

**Goal:** explore result sharing without pretending local files are trusted.

**Deliverables:** result bundle format, optional upload or local history
prototype, validation rules, and leaderboard mock view.

**Acceptance criteria:**

- Submitted metadata is schema-validated and versioned.
- The UI labels unverified local results clearly.
- Replay and result hashes can be compared.
- Hosted user-code execution is not introduced.

**Tests/checks:** valid and invalid submissions, duplicate handling, version
mismatch, and leaderboard rendering.

**Why it matters:** tests social comparison while keeping secure hosted judging
as a separate future project.
