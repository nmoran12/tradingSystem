# OrderBook Arena Roadmap

## Approach

Build two connected pieces:

1. a website for discovering and understanding challenges
2. a local runner for executing strategies and generating results

The MVP does not need accounts, cloud judging, or secure hosted execution.

## Six-Week Plan

### Week 1: Website and Product Skeleton

- Turn the existing frontend into a basic Arena website shell.
- Add challenge list and challenge detail page designs.
- Define how prompts, rules, starter files, and sample results are presented.
- Keep the existing replay viewer reachable.

**Outcome:** users can browse a small static challenge catalogue.

### Week 2: Challenge Definition Schema

- Define one versioned challenge format shared by the website and local runner.
- Add validation and one complete sample challenge.
- Include scenario, risk, scoring, prompt, and starter metadata.

**Outcome:** one challenge definition can drive both presentation and judging.

### Week 3: Local Runner and Baselines

- Implement one deterministic single-instrument scenario.
- Add the local episode runner around the existing matching engine.
- Add no-op and simple active baseline strategies.
- Track fills, cash, inventory, and risk state.

**Outcome:** built-in strategies complete repeatable local episodes.

### Week 4: Scoring and Results

- Implement a transparent scoring formula.
- Write versioned result JSON with raw metrics.
- Add batch runs across a small public seed set.
- Test hand-calculated scores and repeated-run equivalence.

**Outcome:** the CLI produces an explainable score and result file.

### Week 5: Replay and Python Strategy API

- Export strategy actions, engine events, portfolio state, and score data.
- Load generated replay files in the website viewer.
- Add a local child-process Python strategy protocol.
- Handle timeout, crash, malformed response, and invalid action cases.

**Outcome:** a user can run a Python strategy locally and inspect the replay in
the website.

### Week 6: End-to-End Polish and Submission Spike

- Document the full solve flow from challenge page to replay.
- Add sample baseline results and generated replay files to the website.
- Improve CLI errors and starter instructions.
- Optionally prototype result metadata upload or a local leaderboard.
- Record a short demo and run clean verification.

**Outcome:** a focused website-first portfolio demo with local judging.

## MVP Scope

- static challenge catalogue and detail pages
- one shared challenge definition format
- deterministic local CLI runner
- existing C++ matching engine as the judge
- built-in baseline strategies
- scoring and result JSON
- replay export and browser-based replay viewing
- Python strategy API

## Polish After MVP

- More challenge types and starter templates.
- Better Arena-specific replay panels.
- Result history and comparisons across seed sets.
- Improved local runner packaging.
- Benchmarks separating Arena orchestration from raw engine cost.

## Stretch Features

- Experimental result upload and leaderboard.
- Server-held evaluation episodes.
- Process-based C++ strategy SDK.
- Multi-instrument challenges.
- Secure hosted code execution.

Hosted execution remains last because it adds security and operational work
without being necessary for the initial challenge experience.
