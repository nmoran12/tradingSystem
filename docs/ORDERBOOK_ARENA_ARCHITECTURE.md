# OrderBook Arena Architecture

## Design Goal

Separate the website experience from local strategy execution. The website
publishes challenge content and displays replays. The local CLI owns simulation,
strategy processes, scoring, and output generation.

## User Flow

```text
User opens website
-> browses challenges
-> reads prompt, rules, examples, and baseline results
-> downloads or copies a starter strategy
-> runs the Arena CLI locally
-> local CLI runs deterministic scenarios through the C++ engine
-> CLI writes score.json and replay.ndjson
-> user opens replay.ndjson in the website visualiser
-> optional future result upload or leaderboard
```

## System Split

```text
WEBSITE / FRONTEND

Challenge catalogue -> Challenge page -> Starter instructions
                                |
                                v
                         Replay file picker
                                |
                                v
                         Replay visualiser

No user strategy execution in the MVP website.


LOCAL MACHINE

Challenge Definition
        |
        v
Arena CLI <----------> Local Strategy Process
        |
        v
Deterministic Scenario Generator
        |
        v
Existing C++ MatchingEngine -> OrderBook
        |
        +-> Portfolio / Risk State -> Scoring Engine -> score.json
        |
        +-> Versioned Event Log ---------------------> replay.ndjson


OPTIONAL LATER SERVICE

Result metadata upload -> validation -> leaderboard display
```

## Main Components

### Website and Challenge Catalogue

The website is the user's starting point. It should contain static or
repository-backed challenge content:

- challenge title, difficulty, and tags
- prompt and market rules
- input, output, and strategy API explanation
- risk limits and scoring formula
- starter strategy download or code sample
- baseline score and sample replay

The first version can be a static frontend. It does not need accounts, a
database, or a backend.

### Challenge Definitions

A versioned challenge definition is shared by the website build and local
runner. It describes:

- stable ID, title, version, and difficulty
- scenario type and public seed set
- episode length and simulated clock settings
- initial cash and inventory
- order and position limits
- scoring weights
- starter files and explanatory content references

The website reads presentation fields. The local runner validates and uses the
simulation fields. The format contains data, not executable code.

### Local CLI and Strategy Runner

The CLI coordinates each run:

1. load and validate the challenge
2. start a built-in or local user strategy
3. send market observations to the strategy
4. validate returned actions
5. process orders through the matching engine
6. update fills, cash, inventory, and risk state
7. calculate the score
8. write result and replay files

Built-in strategies should establish the contract first. Python strategies can
run as local child processes using a small versioned message protocol. This
keeps failures outside the engine process but is not a security boundary.

### Existing C++ Matching Engine

`MatchingEngine` and `OrderBook` remain responsible for matching and resting
order state. They should not contain challenge prompts, strategy logic, PnL,
website concerns, or scoring policy.

The MVP uses one engine instance for one instrument per episode.

### Deterministic Scenario Generator

The generator owns seeded market activity, simulated timestamps, and episode
termination. It must not depend on wall-clock time, thread scheduling, or
global randomness.

The same challenge version and seed must produce the same canonical scenario
events.

### Scoring and Result JSON

The scoring engine consumes completed portfolio and risk state. A result file
should include:

- challenge and engine versions
- seed or seed-set identity
- final score
- PnL, drawdown, inventory, fills, and penalties
- run status and failure reason when applicable
- replay file reference or content hash

The score must be explainable from the raw metrics.

### Replay Export and Website Viewer

The local runner writes a versioned event log and visualisation-friendly replay
file. The website loads that file in the browser; no upload is required.

The viewer can later show strategy actions, fills, cash, inventory, and score
components alongside the existing order-book views.

### Optional Result Submission and Leaderboard

A later service may accept result metadata and display rankings. A local result
file alone is not proof that a run was honest, so early leaderboards should be
labelled experimental.

Trusted rankings would eventually require server-held episodes or server-side
re-evaluation. That depends on secure hosted execution and is not part of the
MVP.

## Why Hosted Execution Is Deferred

Running arbitrary code on a server introduces work unrelated to the first
product loop:

- process and filesystem isolation
- CPU, memory, and time limits
- dependency and compiler management
- abuse prevention and authentication
- secret evaluation episodes
- reproducible result verification
- hosting and operational support

The MVP can prove challenge quality, deterministic judging, scoring, and replay
without taking on those risks.

## Determinism Rules

- Identify a run by challenge, engine, strategy, protocol, and seed versions.
- Use a simulated clock.
- Explicitly seed every random generator.
- Define event ordering for equal timestamps.
- Specify score arithmetic and rounding.
- Produce stable result and replay schemas.

## Planned Repository Structure

```text
arena/
  challenges/          Versioned challenge definitions and starter files
  strategies/          Built-in baseline strategies
include/arena/          Local runner, scenario, scoring, and result interfaces
src/arena/              Arena implementations
tests/arena/            Unit, golden, and end-to-end tests
ui/replay-visualiser/   Website catalogue and replay views
```

Create these paths only when their implementation milestone starts.

## Open Decisions

- Challenge schema and content format.
- Whether the existing Vite app becomes the full Arena website.
- Strategy observation and action protocol.
- Final PnL marking and score normalization rules.
- Timeout and failure handling for local strategies.
- Result-signing or verification approach for an experimental leaderboard.
