# OrderBook Arena Replay Schema

**Replay schema version:** `1.0`

## Purpose

Arena replay files are deterministic JSON Lines artifacts produced by the local
`execution_v1` evaluator. They explain one evaluated run without requiring the
strategy process to run again.

Each line is one JSON object. A file may contain multiple episodes, ordered by
the configured seed set.

This is a compact challenge replay, not a full exchange audit log.

## Common Record Fields

Every record contains:

| Field | Meaning |
|---|---|
| `replay_schema_version` | Replay contract version. Currently `1.0`. |
| `record_index` | Zero-based position in the complete JSONL file. |
| `episode_sequence` | Zero-based position within one seed's episode. |
| `seed` | Deterministic episode seed. |
| `event_index` | Simulated callback index, or `null` when not applicable. |
| `type` | Record type described below. |

`record_index` and `episode_sequence` must be contiguous. Each episode starts
with `episode_start` and ends with `episode_result`.

The older `sequence` field remains an alias of `episode_sequence`.

## Record Types

### `episode_start`

Identifies the challenge, strategy, runner, simulator, generator, PRNG, and
seed. It also states whether the C++ matching engine was used.

### `market_update`

Describes a deterministic scenario change, including midpoint movement and
which synthetic depth levels were refreshed.

### `book_update`

Contains the visible bid and ask levels supplied to the strategy plus the
portfolio state before the callback:

```json
{
  "type": "book_update",
  "event_index": 3,
  "book": {
    "symbol": "ARENA",
    "timestamp_ms": 300,
    "bids": [{"price_ticks": 9998, "quantity": 74}],
    "asks": [{"price_ticks": 10001, "quantity": 123}]
  },
  "portfolio": {
    "target_quantity": 500,
    "filled_quantity": 169,
    "remaining_quantity": 331,
    "total_cost_tick_units": 1690000,
    "average_fill_price_ticks": 10000.0,
    "open_orders": []
  }
}
```

The example omits common fields and additional visible depth levels.

### `strategy_action`

Records the raw action returned by the strategy and its zero-based
`action_index` within the callback response.

### `action_result`

Records the same action with `status` set to `accepted` or `rejected`.
Rejected actions include a human-readable `reason`.

Acceptance means the action passed the current evaluator checks. Detailed
effects are represented by fill, cancellation, and order records.

### Order and Fill Records

- `order_processed`: market or limit order processing result;
- `order_cancelled`: successful strategy cancellation;
- `order_filled`: resting order fully removed after fills;
- `fill`: order ID, source, price ticks, and quantity.

### `portfolio_update`

Contains portfolio state after all accepted strategy actions for the event.

### `episode_invalid`

Records the reason an episode was invalidated.

### `episode_result`

Closes the episode with:

- `completed`, `incomplete`, or `invalid` status;
- score and all A5 execution metrics;
- final visible book and portfolio state;
- event, action, order, and fill counters;
- deterministic reproduction metadata.

## Result JSON Compatibility

Result schema `1.1` references the replay using:

```json
{
  "replay": {
    "format": "jsonl",
    "schema_version": "1.0",
    "artifact_name": "builtin.replay.jsonl",
    "record_count": 272,
    "seeds": [901, 902, 903]
  }
}
```

`artifact_name` is a filename, not an absolute path. This keeps result files
portable across machines. `record_count` and `seeds` allow a consumer to check
that the selected replay matches the result metadata.

These fields are an additive part of result schema `1.1`; score version `1.0`
and score behavior are unchanged.

## Determinism

For fixed challenge contents, seed set, runner version, and deterministic
strategy, replay records and JSONL serialization are byte-stable.

The replay uses simulated `timestamp_ms` values from the scenario. It does not
include wall-clock generation timestamps.

## Validation

The evaluator validates replay records before writing them. The browser viewer
also rejects:

- invalid JSONL;
- missing or unsupported replay schema versions;
- non-contiguous record or episode ordering;
- missing required book or portfolio state;
- empty files;
- episodes without start or result records.

## Limitations

- The replay reflects `python_level_book_skeleton_v1`, not the C++ matching
  engine.
- It records visible strategy book depth, not every internal simulator state.
- It does not provide cryptographic provenance or trusted submission evidence.
- Local files and bundled seeds are inspectable.
- The visualiser loads artifacts from the user's machine; it does not run,
  submit, or host strategy code.
