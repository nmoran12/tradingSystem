# OrderBook Arena Active Milestone

## A1: Website/Product Skeleton and Challenge Browsing

**Status:** Ready, not started

## Goal

Create the first website structure for browsing OrderBook Arena challenges.
This milestone presents challenge content only; it does not execute strategies
or change the matching engine.

## Planned Tasks

- [ ] Decide whether to extend the existing Vite replay app or add an Arena
      shell around it.
- [ ] Add a simple website layout and navigation.
- [ ] Add static challenge catalogue data for at least two sample challenges.
- [ ] Add a challenge detail view with prompt, rules, constraints, scoring
      summary, starter instructions, and baseline result.
- [ ] Keep the existing replay viewer accessible.
- [ ] State clearly that strategies run through a local CLI, which is planned
      for A3.
- [ ] Add focused frontend tests or schema checks for catalogue data.

## Acceptance Criteria

- [ ] Users can browse challenge cards and open a detail view.
- [ ] A challenge page explains how a future user will run a starter strategy
      locally.
- [ ] No server-side execution, authentication, database, or result upload is
      added.
- [ ] No changes are made to matching semantics.
- [ ] Existing C++ tests and frontend production build still pass.
- [ ] Website copy does not imply production trading or secure hosted judging.

## Checks

```bash
cmake --build build
ctest --test-dir build --output-on-failure
npm run build --prefix ui/replay-visualiser
git diff --check
git status --short --branch
```

## Start Rule

Do not begin implementation until the website-first planning commit has been
reviewed.
