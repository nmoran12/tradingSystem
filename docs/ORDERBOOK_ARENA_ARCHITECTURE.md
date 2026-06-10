# OrderBook Arena Architecture

## Design Goal

OrderBook Arena should present a website-first challenge experience while
keeping challenge execution deterministic and auditable. Python and C++ are the
strategy languages. JavaScript or TypeScript is limited to the website
frontend.

The planned architecture supports two execution paths:

- an early local runner used while the product contracts are being developed;
- a later hosted judge that runs untrusted code in isolated environments.

The current implementation has only the local path described below. The hosted
path and C++ matching-engine integration remain future work.

## Current Implemented Local Flow

```text
Challenge JSON
      |
      v
Local evaluator (`python_level_book_skeleton_v1`)
      |
      +--> built-in strategy callback
      |
      +--> local Python strategy file over JSONL
      |
      +--> trusted local C++ strategy process over JSONL
      |
      v
Scoring engine + result JSON
      |
      v
Replay JSONL
      |
      v
Website result/replay viewer
```

The user runs the CLI and strategy on their own machine, then imports generated
artifacts into the browser. The website does not execute or upload code.

Current boundaries:

- hosted Python execution is not implemented;
- the repository's C++ matching engine is not used by Arena yet;
- the local Python and C++ child processes are trusted local code, not
  sandboxed code;
- bundled seeds and local artifacts are inspectable and unverified;
- there is no backend, hosted judge, account system, or leaderboard.

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

Python and C++ strategies implement equivalent callbacks and receive the same
normalized events. A language-neutral process protocol keeps the judge from
embedding a Python interpreter or loading untrusted native plugins into its
own process.

The implemented local adapters are:

- a local Python strategy file runner;
- a local compiled C++ strategy process.

Each adapter needs explicit message framing, protocol versions, deadlines,
error reporting, and deterministic handling of invalid output.

### Local CLI Runner

The local CLI is the early execution bridge. It currently:

- loads and validates a challenge;
- runs built-in reference strategies, local Python strategy files, or launches
  a compiled C++ strategy;
- runs deterministic public or bundled development episodes locally;
- enforces a per-response timeout for the C++ process;
- produces result JSON and replay files;
- records enough metadata to reproduce a run.

Local execution is not a security boundary because the user runs their own code
on their own machine. Bundled episodes are also inspectable, so local results
cannot provide secret-test integrity.

### Future C++ Judge and Matching Engine

The existing C++ order book is intended to become the simulation engine. That
integration is not implemented in the local MVP. The current evaluator models
a deterministic level book in Python and records
`uses_cpp_matching_engine: false`.

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

## Current Repository Areas

```text
arena/
  challenges/       # versioned challenge definitions
  cpp/              # trusted local C++ strategy interface and example
  protocol/         # JSONL process contract and fixtures
  tools/            # evaluator, process adapter, scoring, replay validation
  tests/             # deterministic evaluator and artifact tests
  results/           # ignored generated result files
  replays/           # ignored generated replay files

ui/replay-visualiser/ # challenge pages and result/replay viewer
docs/               # design, milestones, formats, and experiments
```

Hosted execution services should not be added until their trust boundaries and
deployment model are documented.

## Open Decisions

- hosted Python strategy process design and protocol parity;
- migration from the Python simulator to the C++ matching engine;
- sandbox technology and deployment boundary for the Python spike;
- how hidden scenarios remain private once hosted judging exists;
- whether local result files can be uploaded only for viewing, not ranking.
