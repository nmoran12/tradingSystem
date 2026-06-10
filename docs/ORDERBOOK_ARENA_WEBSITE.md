# OrderBook Arena Website

## Status

The current website is a local React and Vite application for:

- browsing available Arena challenges;
- reading the **Beat the Market Order** prompt and rules;
- viewing Python and C++ callback surfaces;
- following the local evaluator workflow;
- inspecting result JSON score and per-episode metrics;
- inspecting replay JSONL artifacts.

It does not run, upload, or submit strategy code. Hosted judging, sandboxing,
accounts, and leaderboards are not implemented.

## Run Locally

From the repository root:

```bash
cd ui/replay-visualiser
npm ci
npm run dev
```

Open the Vite URL. The application uses hash routes:

| Route | Page |
|---|---|
| `#/challenges` | Challenge browser |
| `#/challenges/beat_market_order` | Challenge prompt and local workflow |
| `#/results` | A8 local result and linked replay viewer |
| `#/replay` | A6 local replay visualiser |

Hash routing keeps the first website milestone deployable as static files
without a server-side route configuration.

## Challenge Data

The frontend imports:

```text
arena/challenges/beat_market_order.v1.json
```

at build time. The challenge list and detail page derive the following fields
from that versioned definition:

- title, difficulty, tags, prompt, and challenge type;
- supported strategy languages and callback names;
- target, event count, market settings, and visible depth;
- allowed actions;
- score version, reported metrics, and output schema versions;
- local evaluation seeds and their inspectable-seed warning.

Explanatory prose, workflow commands, and starter snippets live in the
frontend. They describe how to use the current implementation and are not a
second challenge-definition format.

## Language Status

### C++

Trusted local C++ process execution is available. The starter shown on the
challenge page uses `execution_v1_strategy.hpp` and the existing
`onBookUpdate(const BookView&, const Portfolio&)` callback.

### Python

Trusted local Python strategy-file execution is available. The page shows the
function-based `on_book_update(book, portfolio)` contract and the local CLI
command for running it.

## Local Workflow

The challenge page explains this flow:

1. Build the included C++ example strategy.
2. Evaluate the built-in reference, a local Python strategy file, or the
   compiled C++ process.
3. Generate result JSON and replay JSONL locally.
4. Start the website.
5. Open `#/results` and import the result JSON.
6. Import the matching replay JSONL and inspect a selected episode.

No page presents a hosted Run or Submit control.

## Result and Replay Compatibility

The result viewer accepts result schema `1.1`. It shows challenge and strategy
identity, simulator and generator versions, aggregate metrics, replay metadata,
and a selectable per-episode breakdown.

After a result is loaded, the matching replay may be imported. The viewer
checks:

- replay schema version;
- exact replay record count;
- ordered episode seeds.

A compatible replay opens in the existing A6 inspector at the selected result
seed. These checks detect obvious file mismatches only. Local artifacts remain
unverified and are not evidence of leaderboard eligibility.

## Checks

```bash
cd ui/replay-visualiser
npx tsc --noEmit
npm test
```

`npm test` builds the production bundle and runs focused content checks for:

- challenge list and detail content;
- valid and invalid result parsing;
- result/replay seed and record-count compatibility;
- aggregate and per-episode result surfaces;
- local-only execution messaging;
- challenge, result, and replay routes;
- versioned challenge adapter usage;
- replay parser preservation;
- absence of hosted Run or Submit buttons.

## Limitations

- Only one `execution_v1` challenge exists.
- The frontend has no backend or persistence.
- There is no online editor or hosted strategy execution.
- Hosted Python execution is not implemented.
- Local Python strategy files are available and trusted only on the user'"'"'s
  machine.
- The evaluator still uses `python_level_book_skeleton_v1`.
- Compatibility checks do not prove that a local result was produced honestly.
- Local seeds, results, and replays are inspectable and unverified.
