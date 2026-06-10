# OrderBook Arena Overview

## What It Is

OrderBook Arena is an experimental, LeetCode-style coding challenge website
built around this repository's C++ matching engine.

The product direction is:

- browse a list of trading and market-microstructure challenges
- read a prompt, rules, examples, and baseline results
- download or copy a starter strategy
- run the strategy locally against deterministic scenarios
- inspect the score, metrics, and replay in the website

The website explains and presents challenges. The local CLI performs strategy
execution and simulation.

## Why the Website Exists

The website gives the project a clear way to organise challenges and results.
It should make each challenge understandable without requiring users to read
the C++ source.

For the MVP, it should provide:

- a challenge list
- challenge detail pages
- prompts, rules, constraints, and starter instructions
- sample and baseline results
- a file-based replay viewer
- clear commands for running a solution locally

It does not execute user code.

## What Runs Locally

The local runner should:

1. Load a versioned challenge definition.
2. Start the user's strategy.
3. Generate deterministic single-instrument scenarios.
4. Run orders through the existing C++ matching engine.
5. Track fills, cash, inventory, risk limits, and summary metrics.
6. Calculate a deterministic score.
7. Write result JSON and a replay file.

The same challenge version, strategy version, and seed should produce the same
result.

## How a User Solves a Challenge

```text
Open challenge page
-> read prompt and rules
-> get starter strategy
-> edit and run it locally with the Arena CLI
-> receive score.json and replay.ndjson
-> open the replay in the website visualiser
```

An optional later submission flow may upload signed or reproducible result
metadata for a leaderboard. It should not be confused with trusted evaluation.

## First MVP

The first useful version should include:

- website challenge browsing and detail pages
- one versioned challenge format
- one deterministic scenario type
- a local CLI backed by the existing C++ engine
- built-in baseline strategies
- transparent scoring and result JSON
- replay export and website replay viewing
- a small Python strategy API

## What It Is Not

OrderBook Arena is not:

- a live trading system
- a broker or exchange connection
- a professional quant platform
- a secure server-side code runner
- a claim that local evaluation episodes are truly hidden

Hosted execution is deferred because safely running untrusted code requires
isolation, resource limits, abuse controls, result verification, and ongoing
operations. Those are separate problems from proving the challenge experience.
