# OrderBook Arena Challenge Schema

**Schema version:** `1.0`
**First challenge type:** `execution_v1`

## Scope

This document defines the first OrderBook Arena challenge format. It is
deliberately limited to one task:

> Buy a target quantity during a fixed deterministic episode while reducing
> execution cost relative to an immediate market-order baseline.

The format is shared by future Python and C++ local runners. It does not define
accounts, leaderboards, hosted execution, multiple assets, historical data,
market making, or a general plugin system.

The example challenge is
[`arena/challenges/beat_market_order.v1.json`](../arena/challenges/beat_market_order.v1.json).

## Design Rules

- Prices are signed integer ticks, matching the existing C++ engine.
- Quantities and order IDs are non-negative integers.
- `execution_v1` is single-symbol and buy-only.
- The platform owns market simulation, matching, portfolio tracking, scoring,
  result generation, and replay generation.
- User code implements one function that receives read-only state and returns
  zero or more actions.
- Python and C++ receive equivalent logical inputs and can issue the same
  actions.
- Fields are added only when the first runner needs them.

## Top-Level Fields

| Field | Meaning |
|---|---|
| `schema_version` | Version of this challenge document format. |
| `challenge_id` | Stable lowercase identifier. |
| `title` | Display title. |
| `difficulty` | Display difficulty: initially `easy`, `medium`, or `hard`. |
| `tags` | Short website filtering labels. |
| `prompt` | Human-readable task shown to the user. |
| `challenge_type` | Must be `execution_v1` for this version. |
| `strategy` | Supported languages and callback contract. |
| `allowed_actions` | Actions accepted from user code. |
| `market` | Symbol, units, book visibility, and deterministic generator settings. |
| `task` | Side, target quantity, and episode window. |
| `episodes` | Public, development, and local evaluation seeds. |
| `limits` | Per-episode strategy and order limits. |
| `scoring` | Completion rule, baseline, score formula, and reported metrics. |
| `outputs` | Expected result and replay artifacts. |

Unknown top-level fields should be rejected by the future parser unless the
schema version explicitly permits them.

## Strategy Contract

### Python

```python
def on_book_update(book, portfolio) -> list[Action]:
    ...
```

### C++

```cpp
std::vector<Action> onBookUpdate(
    const BookView& book,
    const Portfolio& portfolio);
```

These are functions, not subclass hooks. The runner invokes the callback once
after initial book construction and after each later scenario book update.
User code does not receive direct access to the matching engine.

### `BookView`

Both languages receive the equivalent of:

| Field | Type | Meaning |
|---|---|---|
| `event_index` | integer | Zero-based callback index. |
| `events_remaining` | integer | Callbacks remaining after the current one. |
| `timestamp_ms` | integer | Deterministic simulated time from episode start. |
| `symbol` | string | Challenge symbol. |
| `bids` | list of price levels | Best bid first, limited by `visible_depth_levels`. |
| `asks` | list of price levels | Best ask first, limited by `visible_depth_levels`. |

Each price level contains `price_ticks` and aggregate `quantity`.

### `Portfolio`

Both languages receive the equivalent of:

| Field | Type | Meaning |
|---|---|---|
| `target_quantity` | integer | Quantity required by the task. |
| `filled_quantity` | integer | Total strategy buy fills so far. |
| `remaining_quantity` | integer | Target quantity not yet filled. |
| `total_cost_tick_units` | integer | Sum of `fill_price_ticks * fill_quantity`. |
| `average_fill_price_ticks` | number or null | Current strategy VWAP. |
| `open_orders` | list | Strategy-owned resting orders and remaining quantities. |

An open order contains `order_id`, `type`, `price_ticks`, and
`remaining_quantity`.

## Allowed Actions

The initial action set is intentionally small.

### `market_order`

```json
{
  "type": "market_order",
  "order_id": 1001,
  "quantity": 50
}
```

Submits a buy market order. Unfilled market quantity is cancelled, matching the
existing engine.

### `limit_order`

```json
{
  "type": "limit_order",
  "order_id": 1002,
  "price_ticks": 9999,
  "quantity": 50
}
```

Submits a buy limit order. Any unfilled quantity may rest in the book.

### `cancel_order`

```json
{
  "type": "cancel_order",
  "order_id": 1002
}
```

Cancels a strategy-owned open order.

Order IDs are positive integers chosen by the strategy and must be unique
within an episode. The runner rejects sell orders, modify actions, duplicate
active order IDs, quantities above the remaining target, and actions outside
the configured limits.

## `execution_v1` Episode Semantics

1. Build the initial deterministic book from the challenge settings and seed.
2. Record the arrival midpoint used for reporting slippage.
3. Invoke the strategy callback at `event_index = 0`.
4. Apply returned actions in list order.
5. Advance the deterministic scenario by one market event.
6. Invoke the callback with the updated book and portfolio.
7. Repeat until `event_count` callbacks have completed.
8. Cancel remaining strategy orders and calculate the episode result.

The strategy may stop returning actions after filling the target. It may not
buy more than the target quantity.

The scenario generator must guarantee enough initial ask liquidity for the
configured immediate-market baseline to complete.

## Deterministic Seed Sets

The challenge file separates seeds by use:

- `public`: examples suitable for prompts, tests, and sample results;
- `development`: additional local regression episodes;
- `local_evaluation`: local held-out episodes used by the CLI.

All files and seeds distributed to a user's machine are inspectable. The local
evaluation set is useful for repeatable testing but is not secret and cannot
support a trusted public leaderboard.

Future hosted judging may map `private_evaluation_set` to undistributed seeds.
That field is disabled in the example and does not imply hosted execution
exists.

Determinism requires the generator name, generator version, challenge schema
version, engine version, and seed to be recorded in result and replay output.

## Limits

The first schema supports:

- maximum submitted orders per episode;
- maximum simultaneously open orders;
- maximum actions returned by one callback;
- maximum total actions per episode;
- maximum quantity for one order;
- local callback timeout placeholder;
- future hosted callback timeout placeholder.

Timeout fields describe the intended runner contract. A2 does not implement or
enforce them.

Limit violations should produce a structured invalid-run result rather than a
normal score.

## Scoring

The baseline strategy is `immediate_market_order_v1`: submit one buy market
order for the full target quantity at the first callback.

For each completed episode:

```text
strategy_vwap_ticks = total strategy cost / target quantity
baseline_vwap_ticks = total baseline cost / target quantity
episode_score = baseline_vwap_ticks - strategy_vwap_ticks
```

A positive score means the strategy bought more cheaply than the baseline. A
negative score means it performed worse. The overall score is the arithmetic
mean of all selected episode scores.

The challenge JSON selects the named metric
`vwap_improvement_ticks`; it does not contain an executable formula language.

Full completion is required. An incomplete or invalid episode receives a score
of zero and must report the reason and remaining quantity. This prevents a
strategy from appearing successful by buying only a small, cheap fraction of
the target.

Result output also reports:

- completion ratio;
- strategy and baseline VWAP;
- strategy and baseline slippage from arrival midpoint;
- execution-cost improvement;
- filled and remaining quantity;
- order and action counts;
- maximum open orders;
- invalid-run reason, when applicable.

Drawdown and trading PnL are not primary metrics for this execution task. They
should not be added until a challenge has inventory held through changing
prices and a precise mark-to-market definition.

## Output Artifacts

### Result JSON

The result file should contain:

- result schema version and status;
- challenge, engine, generator, and strategy API versions;
- language and strategy identifier;
- seed set and per-episode seeds;
- overall score and per-episode metrics;
- baseline metrics;
- limit or callback failures;
- replay file reference.

### Replay JSONL

The replay should contain ordered records for:

- episode metadata;
- book updates;
- strategy callbacks;
- submitted actions;
- accepted and rejected orders;
- fills and cancellations;
- portfolio updates;
- final metrics.

A2 defines expected content only. Replay and result schemas are implemented in
later milestones.

## Validation Rules for the Future Parser

- `schema_version` must be supported.
- `challenge_type` must be `execution_v1`.
- `strategy.supported_languages` must contain `python` and `cpp`.
- Callback names must match the language contract.
- Only the three documented actions are allowed.
- All seeds must be non-negative integers and unique across local sets.
- Target quantity, event count, depth, and order limits must be positive.
- Initial visible ask liquidity must be at least the target quantity.
- `task.side` must be `buy`.
- The configured baseline and named score metric must be supported.
- Output schema versions and filenames must be present.

The parser and these validation checks belong to the runner milestone, not A2.
