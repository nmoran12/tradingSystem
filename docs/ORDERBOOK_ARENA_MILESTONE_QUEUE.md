# OrderBook Arena Milestone Queue

Milestones are ordered by dependency and resume value. Each one should leave a
small, testable result. Hosted execution is intentionally after the local
challenge loop and website viewer.

## A1. Product and Documentation Alignment

**Goal:** Define the website-led Python/C++ product direction before changing
the implementation.

**Deliverables**

- aligned overview, architecture, roadmap, milestone, and README documents;
- clear distinction between the local execution bridge and hosted target;
- documented security boundary for future untrusted execution;
- initial list of product and architecture decisions still open.

**Acceptance criteria**

- Python and C++ are consistently named as strategy languages;
- JavaScript or TypeScript is described only as frontend technology;
- no document claims hosted execution is implemented;
- local execution is described as temporary;
- hosted Python is ordered before hosted C++.

**Tests/checks**

- review all Arena docs for contradictory scope;
- run Markdown link and formatting checks if available;
- inspect `git diff --check`.

**Why it matters:** It prevents incompatible runners, schemas, and website work
from being built around different product assumptions.

## A2. Challenge Definition Schema

**Goal:** Define the smallest versioned challenge contract needed by the first
local runner.

**Deliverables**

- `execution_v1` schema documentation;
- one valid **Beat the Market Order** example challenge;
- function-based Python and C++ callback signatures;
- minimal book, portfolio, action, seed, limit, scoring, and output fields.

**Acceptance criteria**

- the example is valid JSON;
- schema, engine, generator, strategy API, scoring, and output versions are
  explicit;
- local seed sets are identified as inspectable;
- full-completion execution scoring is defined against an immediate-market
  baseline;
- no parser, runner, or strategy execution code is added.

**Tests/checks**

- `python3 -m json.tool arena/challenges/beat_market_order.v1.json`;
- `git diff --check`;
- documentation review against the A2 acceptance criteria.

**Why it matters:** A stable challenge contract is the foundation for both
languages, the website, scoring, and hosted judging.

## A3. Local `execution_v1` Evaluator Skeleton

**Goal:** Prove deterministic challenge evaluation before running external user
code.

**Deliverables**

- challenge loading and validation;
- explicit SplitMix64 scenario generation;
- built-in immediate-market and reference strategies;
- strict action handling, scoring, result JSON, and replay JSONL;
- deterministic single-challenge CLI and focused tests.

**Acceptance criteria**

- both built-in strategies complete the example challenge;
- repeated runs with the same seed match;
- invalid and incomplete episodes receive zero;
- every local evaluation seed contributes to the aggregate;
- no external strategy process or sandbox is added.

**Tests/checks**

- config loading and deterministic episode tests;
- invalid-action and incomplete-episode tests;
- local seed aggregation test;
- evaluator CLI smoke test.

**Why it matters:** It validates the challenge, scoring, and artifact contracts
before process execution adds another failure boundary.

## A4. Local C++ Strategy Runner

**Goal:** Give C++ strategies the same challenge semantics through a separate
compiled process.

**Deliverables**

- function-based C++ starter interface and example strategy;
- standalone CMake build;
- trusted-local JSON Lines process adapter;
- normalized request/response fixtures;
- timeout and clear protocol/process failures.

**Acceptance criteria**

- the C++ reference completes the example challenge;
- repeated C++ runs are deterministic;
- built-in strategy mode remains unchanged;
- normalized payloads match versioned fixtures;
- invalid JSON, malformed envelopes, exit, and timeout invalidate episodes;
- docs state that the process is trusted local code and the C++ matching engine
  is not used.

**Tests/checks**

- standalone CMake build;
- protocol fixture and deterministic result/replay tests;
- invalid JSON, malformed envelope, exit, and timeout tests;
- existing Arena and C++ repository suites.

**Why it matters:** Native strategy support makes the platform relevant to
systems and low-latency candidates without coupling strategy code to the judge.

## A5. Scoring Engine and Result JSON

**Goal:** Produce explainable, versioned results from deterministic episodes.

**Deliverables**

- scoring rules for the example challenge;
- VWAP, slippage, fill-rate, completion, and baseline-improvement metrics;
- versioned result JSON schema;
- per-episode results and reproduction metadata.

**Acceptance criteria**

- metric definitions and units are documented;
- the same run produces byte-stable or semantically identical result data;
- invalid runs cannot receive a normal score;
- score changes require an explicit score version change.

**Tests/checks**

- hand-calculated metric unit tests;
- golden-result integration tests;
- zero-fill, partial-fill, completed, invalid, and incomplete cases;
- built-in and C++ strategy result regression checks.

**Why it matters:** Supporting metrics make scoring credible and help users
understand trade-offs instead of optimizing an unexplained number.

## A6. Replay Export and Replay Visualiser

**Goal:** Make every scored run inspectable.

**Deliverables**

- versioned replay schema;
- export of market events, actions, fills, book state, and account changes;
- browser replay loader and timeline;
- sample built-in and C++ replay files.

**Acceptance criteria**

- both language runners produce viewable replays;
- replay order is deterministic;
- the viewer shows enough state to explain key fills and metric changes;
- incompatible replay versions fail visibly.

**Tests/checks**

- replay schema validation;
- golden replay fixture;
- viewer smoke test with both sample files.

**Why it matters:** Replay turns the engine into a demonstrable product and
makes challenge outcomes auditable.

## A7. Website Challenge Browser and Prompt Pages

**Goal:** Establish the website as the main product surface.

**Deliverables**

- challenge list and detail pages;
- prompt, rules, metrics, examples, and difficulty display;
- Python and C++ starter templates;
- local workflow and replay-viewer links;
- challenge content imported from the versioned definition.

**Acceptance criteria**

- users can find and understand the example challenge;
- both language interfaces are visible and consistent with the local APIs;
- no Run or Submit control falsely implies hosted execution works;
- challenge content is sourced from versioned definitions or generated metadata.

**Tests/checks**

- frontend build and lint;
- route and content smoke tests;
- manual review at narrow and desktop widths.

**Why it matters:** It demonstrates the intended LeetCode-style experience
without hiding the current execution limitation.

## A8. Website Result and Replay Viewer

**Goal:** Let users inspect artifacts generated by the local runner in the
website.

**Deliverables**

- local result/replay file import;
- score and metric summary;
- per-episode breakdown;
- replay visualisation linked to result metadata.

**Acceptance criteria**

- a user can run locally and inspect the output without editing files;
- invalid or incompatible files show useful errors;
- the page distinguishes local, unverified results from future verified runs.

**Tests/checks**

- frontend fixture tests;
- result/replay compatibility tests;
- end-to-end local run to browser-viewer check.

**Why it matters:** It closes the early product loop and keeps the website
central while execution remains local.

## A8.5. Local MVP Polish and Demo Hardening

**Goal:** Make the completed local loop easy to reproduce and evaluate from a
fresh checkout before hosted-execution work begins.

**Deliverables**

- one end-to-end local demo check script;
- concise reviewer-facing README workflow;
- current local architecture diagram and explicit implementation boundaries;
- consistent local-only website and documentation language.

**Acceptance criteria**

- one command validates, tests, builds, evaluates, and builds the website;
- generated result/replay files remain ignored;
- docs distinguish implemented C++ local execution from planned Python and
  hosted execution;
- no page or document implies local results are verified.

**Why it matters:** It makes the existing MVP demonstrable without increasing
product scope or weakening the hosted-execution security boundary.

## A9. Hosted Python Sandbox Spike

**Goal:** Test whether isolated Python execution can support the final Run and
Submit experience.

**Deliverables**

- threat model and trust-boundary diagram;
- isolated Python job prototype;
- CPU, wall-clock, memory, process, filesystem, network, and output limits;
- queue, cleanup, logging, and dependency policy notes;
- measurements and known bypass risks.

**Acceptance criteria**

- jobs cannot access the web service process or other job workspaces;
- network and filesystem policies are verified;
- process trees are terminated on timeout;
- the spike is explicitly labelled non-production unless independently hardened.

**Tests/checks**

- timeout, fork/process, memory, disk, output, and network abuse cases;
- concurrent isolation tests;
- cleanup and stale-job tests.

**Why it matters:** Hosted Python removes the download step, but only if
untrusted execution is treated as a security boundary.

## A10. Hosted C++ Sandbox Spike

**Goal:** Extend the isolated judge model to compilation and execution of
untrusted C++.

**Deliverables**

- pinned compiler and standard library environment;
- separate compile and run limits;
- binary and workspace lifecycle controls;
- native-code threat-model additions;
- compatibility tests using the local C++ strategy contract.

**Acceptance criteria**

- compile bombs and runtime abuse are bounded;
- generated binaries cannot escape the job boundary;
- compiler diagnostics are returned without leaking host details;
- the prototype reuses the same result and replay pipeline.

**Tests/checks**

- compile timeout and memory cases;
- malicious process, filesystem, network, and output cases;
- toolchain reproducibility test.

**Why it matters:** C++ is a core target language, but native compilation and
execution require more controls than the Python spike.

## A11. Accounts and Leaderboard

**Goal:** Add persistent, trusted competition only after hosted judging is
credible.

**Deliverables**

- accounts and submission history;
- server-verified result provenance;
- challenge/version-specific leaderboard;
- rerun and invalidation policy.

**Acceptance criteria**

- local result files cannot create ranked entries;
- rankings separate challenge and scoring versions;
- users can inspect the result and replay behind their own submissions;
- abuse and retention rules are documented.

**Tests/checks**

- authentication and authorization tests;
- duplicate, replayed, and invalid submission cases;
- ranking and version-partition tests.

**Why it matters:** A leaderboard is useful only when results are comparable and
generated by a trusted judge.

## A12. Public Demo Polish

**Goal:** Make the project easy to evaluate from GitHub and a public demo.

**Deliverables**

- concise README and architecture diagram;
- curated challenge set and starter strategies;
- screenshots and short end-to-end demo;
- reproducible build, test, benchmark, and local-run instructions;
- documented limitations and hosted-execution status.

**Acceptance criteria**

- a fresh checkout can reproduce the documented local flow;
- CI covers the supported build and core tests;
- performance claims name hardware, build mode, workload, and methodology;
- the demo does not overstate security, users, or production readiness.

**Tests/checks**

- clean-environment setup rehearsal;
- full CI and documentation-link check;
- benchmark reproduction and demo smoke test.

**Why it matters:** Strong implementation work has little resume value if a
reviewer cannot understand or reproduce it quickly.
