# Active Milestone: A8.5 Local MVP Polish and Demo Hardening

**Status:** Complete

## Previous Milestone

A8, Website Result and Replay Viewer, was committed as `8168277`:

```text
feat: add arena result viewer
```

## Goal

Make the local-only Arena MVP easy to understand, reproduce, and demonstrate
from a fresh checkout before any hosted sandbox work begins.

## Implemented Scope

- [x] One-command local MVP check at
  `scripts/arena_local_demo_check.sh`.
- [x] Reviewer-oriented README with current features, boundaries, architecture,
  evaluator commands, website workflow, and checks.
- [x] Current architecture separated from future hosted and C++ engine work.
- [x] Documentation corrected to state that external Python strategy execution
  is not implemented.
- [x] Documentation corrected to state that Arena still uses
  `python_level_book_skeleton_v1`.
- [x] Website copy reviewed for local CLI, trusted C++, planned Python, and
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
      +--> trusted local C++ strategy process
      |
      v
Result JSON + replay JSONL
      |
      v
Website result/replay viewer
```

The website does not execute code. Local results are inspectable and
unverified. Hosted judging, sandboxing, accounts, leaderboards, external Python
execution, and C++ matching-engine integration remain outside this milestone.

## Check

```bash
./scripts/arena_local_demo_check.sh
```

The script validates the challenge, runs Arena tests, builds the C++ example,
generates both evaluator artifact pairs, installs locked frontend dependencies,
type-checks the frontend, and runs its production build/tests.

## Exit Condition

A8.5 is committed separately. A9 hosted Python sandboxing has not started.

## Next Planned Milestone

A8.6, Local Python Strategy Runner, is the next planned milestone. It remains
unimplemented until the local Python adapter is added.
