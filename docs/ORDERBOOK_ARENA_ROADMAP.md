# OrderBook Arena Roadmap

## Product Direction

The target product is a website where users solve trading and market-structure
challenges by writing Python or C++ strategies. The C++ order book is the core
judge. The website presents prompts, an editor, results, metrics, and replays.

The implemented local bridge currently supports built-in strategies and a
trusted C++ process. External Python strategy execution remains planned. Hosted
code execution is deferred until the challenge and strategy contracts are
stable and a credible sandbox design exists.

## First Six Weeks

The six-week target is a coherent local judging loop plus the website surfaces
needed to browse challenges and inspect results. It does not include public
hosted execution.

### Week 1: Align the Product and Define the Contract

- finish the Python/C++ product and architecture documentation;
- define a versioned challenge schema;
- create one small example challenge with public and held-out development seed
  sets, acknowledging that both are inspectable locally;
- decide the language-neutral strategy message protocol;
- document deterministic time and random-number rules.

Outcome: one validated challenge definition and a stable enough interface to
start both language runners.

### Week 2: Local Python Runner (Deferred)

- run a Python strategy in a child process;
- exchange normalized market events and actions over the strategy protocol;
- enforce deadlines, message limits, and clear failure results;
- prove repeated runs produce the same engine events and result data.

Planned outcome: a Python strategy can complete one deterministic challenge
locally. This adapter has not been implemented.

### Week 3: Local C++ Runner (Implemented)

- provide a C++ starter strategy using the same logical callbacks;
- compile and run it as a separate process;
- reuse the same framing, validation, and failure semantics;
- add parity tests showing Python and C++ baselines receive equivalent inputs.

Current outcome: the trusted local C++ process is implemented. Python remains
contract-only, so the language-parity outcome is still open.

### Week 4: Scoring and Result JSON

- implement documented VWAP, slippage, fill-rate, completion, and
  baseline-improvement metrics for the example challenge;
- define a versioned result JSON schema;
- add per-episode breakdowns and reproduction metadata;
- create deterministic golden-result tests.

Outcome: runs produce explainable, comparable result files rather than only a
single score.

### Week 5: Replay Export and Visualisation

- export strategy actions, fills, book updates, positions, and metric changes;
- version the replay format;
- connect the existing visualiser or a small website viewer to replay files;
- verify that built-in and local C++ runs can be inspected in the browser.

Outcome: users can understand why a strategy received its result.

### Week 6: Website Challenge and Result Pages

- add challenge browsing and prompt pages;
- show Python and C++ starter interfaces;
- add an editor-shaped UI for the intended product flow without claiming hosted
  Run/Submit works;
- support loading local result and replay files;
- publish a short end-to-end demo and reproducible commands.

Outcome: the repository demonstrates the final website direction while using
the local runner honestly.

## MVP Boundary

The completed local MVP includes milestones A1 through A8:

- versioned challenges;
- built-in and trusted local C++ execution;
- deterministic scoring and results;
- replay export and browser viewing;
- website challenge, prompt, result, and replay pages.

The MVP does not include external Python strategy execution, C++ matching-engine
integration, secure hosted execution, accounts, trusted public submissions, or
a leaderboard. The week-by-week plan above records the original intended
sequence; implementation prioritized the evaluator skeleton and C++ process
before an external Python adapter.

## Post-MVP Hosted Execution

### Hosted Python Sandbox Spike

Investigate a separate execution service for untrusted Python submissions.
Define isolation, resource limits, network and filesystem restrictions, job
cleanup, dependency policy, abuse controls, and deployment boundaries. The
spike should end with a written threat model and measured prototype, not a
claim of production readiness.

Python comes first because it avoids an untrusted compile step and has a smaller
initial toolchain surface than C++.

### Hosted C++ Sandbox Spike

Extend the execution model to compile and run untrusted C++. Account for
compiler CPU and memory use, generated binaries, compile-time limits, toolchain
pinning, and the larger native-code attack surface. Reuse the same challenge,
strategy, result, and replay contracts.

### Accounts and Leaderboard

Only hosted, server-verified results should affect a public leaderboard. Add
accounts and ranking after execution integrity, challenge versioning, and
result provenance are credible.

### Public Demo Polish

Finish concise documentation, screenshots, a recorded end-to-end run,
benchmark methodology, CI checks, and a small curated challenge set. Report
measured limits and known gaps.

## Scope Rules

- Python and C++ are the strategy languages.
- JavaScript or TypeScript is for the website frontend only.
- The C++ matching engine is intended to become the judge and simulation core.
- The local runner is transitional, not the final product experience.
- Local result uploads are not trusted leaderboard submissions.
- Hosted execution must not share a trust boundary with the public web service.
- Performance and security claims require reproducible evidence.
