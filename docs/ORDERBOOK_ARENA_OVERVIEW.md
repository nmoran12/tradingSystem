# OrderBook Arena Overview

## What It Is

OrderBook Arena is a planned LeetCode-style website for trading and
market-structure coding challenges.

Users will browse a challenge, write a strategy in Python or C++, and run it
against deterministic market scenarios judged by the existing C++ order book.
The result should include a score, trading metrics, and a replay that explains
what happened.

JavaScript or TypeScript may power the website frontend. They are not planned
as strategy languages.

## Target User Experience

The intended final flow is:

1. Open the website and choose a challenge.
2. Read the prompt, rules, strategy interface, and sample results.
3. Write a Python or C++ strategy in the online editor.
4. Click **Run** or **Submit**.
5. Run the strategy in an isolated server-side environment against deterministic
   scenarios and hidden episodes.
6. View the score, PnL-style metrics, slippage, fill rate, drawdown, and replay.
7. Optionally compare a submitted result on a future leaderboard.

This seamless flow requires secure hosted execution. That capability does not
exist yet.

## Early MVP

The website remains the product direction, but the first implementation may use
a local runner as a development bridge:

1. The user opens a challenge on the website.
2. The user downloads or checks out starter code for Python or C++.
3. A local CLI runs the strategy against the same challenge definition and C++
   simulation engine.
4. The CLI writes a result JSON file and replay file.
5. The user opens those files in the website result and replay viewer.

The local runner avoids exposing a server to arbitrary user code while the
challenge format, strategy API, scoring, and replay contracts are still being
developed. It is not the final user experience.

## Why Build It

The current repository already contains the hardest domain-specific component:
a C++ limit order book and matching engine. Arena gives that engine a clear
product use:

- challenges define repeatable market situations;
- Python and C++ strategies react to the same event model;
- the engine applies orders and produces an auditable event stream;
- scoring turns each run into comparable results;
- replays make strategy behaviour visible instead of reducing it to one number.

This direction demonstrates both systems engineering and a usable application
without pretending the project is a live trading platform.

## First Useful Scope

The first useful version should provide:

- a small set of versioned challenge definitions;
- deterministic public scenarios and a format that can later reference private
  hosted evaluation episodes;
- local Python and C++ strategy runners using the same contract;
- the existing C++ matching engine as the simulation judge;
- baseline strategies for validating challenge difficulty;
- result JSON containing score and supporting metrics;
- replay export and a browser-based replay viewer;
- website pages for browsing challenges and reading prompts.

Hosted execution should begin only as a separate Python sandbox spike. C++
hosted execution follows later because compiling and running native code safely
adds more operational and security complexity.

## What It Is Not

The early project is not:

- a live trading system;
- an exchange or exchange-grade simulator;
- a brokerage or professional quant platform;
- a claim that PnL from synthetic scenarios predicts real trading performance;
- a cloud judge with secure arbitrary-code execution;
- a public competitive platform with real users.

Accounts, leaderboards, result submission, and hosted Python/C++ execution are
later stages. Any hosted runner must enforce isolation, timeouts, memory and
process limits, restricted filesystem and network access, and reliable cleanup
before it can be treated as a public feature.

Scenarios distributed with the local runner are inspectable, even if they are
labelled as held-out development cases. Truly hidden evaluation requires a
trusted hosted judge.
