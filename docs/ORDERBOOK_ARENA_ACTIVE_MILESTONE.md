# Active Milestone: A8.6 Local Python Strategy Runner

**Status:** Complete

## Previous Milestone

A8.5, Local MVP Polish and Demo Hardening, was committed as `bfefc8e9`:

```text
docs: polish arena local mvp workflow
```

## Goal

Make the local-only Arena MVP complete for both target strategy languages
before any hosted sandbox work begins.

## Implemented Scope

- [x] One-command local MVP check at
  `scripts/arena_local_demo_check.sh`.
- [x] Reviewer-oriented README with current features, boundaries, architecture,
  evaluator commands, website workflow, and checks.
- [x] Current architecture separated from future hosted and C++ engine work.
- [x] Documentation corrected to state that local Python strategy files are
  available and hosted execution is not implemented.
- [x] Documentation corrected to state that Arena still uses
  `python_level_book_skeleton_v1`.
- [x] Website copy reviewed for local CLI, trusted C++, local Python, and
  unverified artifact boundaries.
- [x] Deterministic UI samples kept under
  `ui/replay-visualiser/public/`.
- [x] Generated `arena/results/*.json` and `arena/replays/*.jsonl` remain
  ignored.

## Local MVP Flow

```text
Challenge JSON
      |
      v
Local evaluator
      |
      +--> built-in strategy
      +--> local Python strategy file
      +--> trusted local C++ strategy process
      |
      v
Result JSON + replay JSONL
      |
      v
Website result/replay viewer
```

The website does not execute code. Local results are inspectable and
unverified. Hosted judging, sandboxing, accounts, leaderboards, hosted Python
execution, and C++ matching-engine integration remain outside this milestone.

## Check

```bash
./scripts/arena_local_demo_check.sh
```

The script validates the challenge, runs Arena tests, builds the C++ example,
generates both evaluator artifact pairs, installs locked frontend dependencies,
type-checks the frontend, and runs its production build/tests.

## Exit Condition

A8.6 is committed separately. A9 hosted Python sandboxing has not started.

## Next Planned Milestone

A8.7, Strategy Authoring Docs and Templates, is the next planned milestone.
