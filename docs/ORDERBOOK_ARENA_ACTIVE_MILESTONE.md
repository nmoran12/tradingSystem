# Active Milestone: A2 Challenge Definition Schema

**Status:** Ready for review

## Previous Milestone

A1, Product and Documentation Alignment, was approved and committed as
`33bc197` with commit message:

```text
docs: align orderbook arena product direction
```

## Goal

Define the smallest challenge contract needed for the first Arena task:
`execution_v1`.

The example challenge, **Beat the Market Order**, asks a Python or C++ strategy
to buy a target quantity during a fixed deterministic episode while improving
on an immediate market-order baseline.

This milestone defines documentation and example JSON only. It does not
implement parsing, scenario generation, scoring, replay export, or strategy
execution.

## Tasks

- [x] Define the `execution_v1` challenge scope and non-goals.
- [x] Define equivalent function-based Python and C++ callbacks.
- [x] Define the minimal `BookView`, `Portfolio`, and `Action` contract.
- [x] Define market, task, episode seed, limit, scoring, and output fields.
- [x] Document that local evaluation seeds are inspectable.
- [x] Define the immediate-market baseline and full-completion scoring rule.
- [x] Add one valid example challenge JSON.
- [x] Link the schema documentation from the README.
- [x] Validate the example with `python3 -m json.tool`.
- [ ] Review and approve the A2 diff.
- [ ] Commit the approved A2 changes.

## Acceptance Criteria

- the example challenge type is exactly `execution_v1`;
- Python and C++ use one small function-based decision interface;
- user code owns only strategy decisions;
- the platform owns the engine, book state, portfolio, scoring, result, and
  replay generation;
- the action set is limited to market order, limit order, and cancel order;
- the task is single-symbol, buy-only, and requires full completion;
- deterministic seed groups and their local visibility are explicit;
- the scoring rule compares strategy VWAP with an immediate-market baseline;
- no runner, website, hosted execution, account, or leaderboard code is added;
- the example file is valid JSON and `git diff --check` passes.

## Checks

```bash
python3 -m json.tool arena/challenges/beat_market_order.v1.json
git diff --check
git status --short --branch
```

## Exit Condition

The user approves the schema and example challenge, then the A2 documentation
changes are committed. A3 may then implement the local Python runner against
this contract.
