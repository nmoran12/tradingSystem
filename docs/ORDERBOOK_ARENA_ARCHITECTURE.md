# OrderBook Arena Architecture

## Design Goal

OrderBook Arena should present a website-first challenge experience while
keeping challenge execution deterministic and auditable. Python and C++ are the
strategy languages. JavaScript or TypeScript is limited to the website
frontend.

The architecture should support two execution paths:

- an early local runner used while the product contracts are being developed;
- a later hosted judge that runs untrusted code in isolated environments.

Both paths must use the same challenge definitions, strategy protocol, C++
matching engine, scoring rules, result format, and replay format.

## Early Local Flow

```text
User opens website
        |
        v
Reads prompt and chooses Python or C++ starter
        |
        v
Runs strategy with local CLI
        |
        v
Local strategy process <-> C++ challenge runner and matching engine
        |
        v
Result JSON + replay file
        |
        v
Website result/replay viewer
```

This flow is a temporary bridge. It allows the engine, challenge model, strategy
APIs, scoring, and visualiser to be built without accepting arbitrary code on a
server.

## Target Hosted Flow

```text
Website prompt + Python/C++ editor
        |
        v
Run / Submit API
        |
        v
Job queue and execution coordinator
        |
        +--> Python sandbox (first)
        |
        +--> C++ compile and run sandbox (later)
        |
        v
C++ challenge runner and matching engine
        |
        v
Scoring engine + result/replay storage
        |
        v
Website metrics, replay, and optional leaderboard
```

The hosted path is a later feature, not an implemented capability.

## Main Components

### Challenge Website

The website is the main product surface. It should provide:

- challenge browsing and prompt pages;
- Python and C++ starter templates;
- an online editor in the target hosted experience;
- sample results and metric explanations;
- result and replay viewing;
- optional later submission history and leaderboards.

Frontend JavaScript or TypeScript renders the interface. It does not replace the
Python or C++ strategy APIs.

### Challenge Definitions

A versioned challenge file should describe:

- challenge identity and schema version;
- prompt and strategy interface version;
- scenario generator and deterministic seeds;
- public sample episodes and references to future private evaluation episodes;
- initial market and account state;
- allowed actions and risk limits;
- scoring rules and metric definitions;
- engine, result, and replay format versions.

The local and hosted runners must interpret the same definition.

### Deterministic Scenario Generator

The scenario generator produces repeatable market events from a challenge
definition and seed. A replay must record enough metadata to reproduce the run.
Determinism should be tested across repeated runs on supported platforms.

### Strategy Interfaces

Python and C++ strategies should implement equivalent callbacks and receive the
same normalized events. A language-neutral process protocol is preferable so
the judge does not embed a Python interpreter or load untrusted native plugins
into its own process.

The first adapters are:

- a local Python strategy process;
- a local compiled C++ strategy process.

Each adapter needs explicit message framing, protocol versions, deadlines,
error reporting, and deterministic handling of invalid output.

### Local CLI Runner

The local CLI is the early execution bridge. It should:

- load and validate a challenge;
- launch a Python strategy or compiled C++ strategy as a child process;
- run deterministic public or bundled development episodes locally;
- enforce basic timeouts and output limits;
- produce result JSON and replay files;
- print enough metadata to reproduce a run.

Local execution is not a security boundary because the user runs their own code
on their own machine. Bundled episodes are also inspectable, so local results
cannot provide secret-test integrity.

### C++ Judge and Matching Engine

The existing C++ order book remains the core simulation engine. The challenge
runner should translate scenario events and strategy actions into engine
operations, then expose fills, book updates, positions, and account state to the
scoring and replay components.

The engine should not contain website, account, or leaderboard logic.

### Scoring Engine

Scoring should be deterministic and challenge-specific. Result JSON may include:

- total score and score version;
- PnL-style outcome for the synthetic episode;
- slippage;
- fill rate;
- maximum drawdown;
- inventory or risk-limit violations;
- per-episode breakdowns;
- challenge, seed, engine, strategy API, and runner versions.

Metric definitions must be documented. A single score without supporting
measurements is not enough to explain a result.

### Replay and Event Log

The runner should write an append-only replay containing market events, strategy
actions, fills, account changes, and timestamps or sequence numbers. The
browser visualiser consumes this file and should work for both local and future
hosted runs.

### Hosted Execution Coordinator

Hosted execution is required for the final no-download experience, but it must
be treated as a security project rather than a normal subprocess wrapper.

At minimum it requires:

- isolation between jobs and from the host;
- disabled or tightly restricted network access;
- CPU, wall-clock, memory, process, file, and output limits;
- restricted filesystem access and temporary workspaces;
- fixed compiler/interpreter and dependency versions;
- controlled compilation for C++;
- termination and cleanup of process trees;
- queue limits, abuse controls, logging, and operational monitoring;
- separation of untrusted execution from web and data services.

Python hosted execution should be investigated first. C++ follows because native
compilation, compiler resource use, generated binaries, and lower-level system
access increase the attack surface. Neither sandbox should be considered
production-ready after a single spike.

### Optional Result Submission and Leaderboard

A later submission service could accept signed result bundles from the hosted
judge and rank results by challenge and version. Results produced only by a
local runner cannot be trusted for a public leaderboard without server-side
verification.

## Planned Repository Areas

Exact names may change after the first schema spike.

```text
arena/
  challenges/       # versioned challenge definitions and prompts
  strategies/       # Python and C++ starter and baseline strategies
  runner/           # local runner and language adapters
  scoring/          # metric and score calculation
  replay/           # replay schema and export
  schemas/          # challenge, result, and replay schemas

ui/                 # challenge pages and result/replay viewer
docs/               # design, milestones, formats, and experiments
```

Hosted execution services should not be added until their trust boundaries and
deployment model are documented.

## Open Decisions

- process protocol and serialization format shared by Python and C++;
- strategy callback surface and action model;
- whether C++ starter strategies are built by CMake or a dedicated CLI command;
- result and replay schema versioning;
- deterministic clock and random-number rules;
- sandbox technology and deployment boundary for the Python spike;
- how hidden scenarios remain private once hosted judging exists;
- whether local result files can be uploaded only for viewing, not ranking.
