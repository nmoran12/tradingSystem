# `execution_v1` Local Strategy Process Protocol

**Version:** `1.0`
**Transport:** JSON Lines over child-process stdin/stdout
**Security boundary:** trusted local execution only

The evaluator starts one C++ process for the full evaluation. Each protocol
message is one JSON object followed by `\n`. The strategy must write exactly
one response line for every `book_update` and flush stdout.

This protocol does not sandbox code. The child process has the same local user
permissions as the evaluator.

## Evaluator to Strategy

### `book_update`

```json
{
  "type": "book_update",
  "protocol_version": "1.0",
  "episode_seed": 123,
  "event_index": 12,
  "events_remaining": 50,
  "timestamp_ms": 1200,
  "symbol": "ARENA",
  "book": {
    "bids": [{"price_ticks": 99, "quantity": 100}],
    "asks": [{"price_ticks": 101, "quantity": 100}]
  },
  "portfolio": {
    "target_quantity": 500,
    "filled_quantity": 120,
    "remaining_quantity": 380,
    "cash_spent_ticks": 12120,
    "average_fill_price_ticks": 101,
    "open_orders": [
      {
        "order_id": 1,
        "side": "buy",
        "type": "limit_order",
        "price_ticks": 99,
        "quantity": 100
      }
    ]
  }
}
```

The evaluator owns these normalized values. The C++ strategy must not assume
access to the simulator or matching engine.

### `episode_end`

```json
{"type":"episode_end","protocol_version":"1.0","episode_seed":123}
```

No response is required. Strategy code may clear episode-local state.

### `evaluation_end`

```json
{"type":"evaluation_end","protocol_version":"1.0"}
```

No response is required. The process should exit with status zero.

## Strategy to Evaluator

For each `book_update`, return:

```json
{
  "type": "actions",
  "actions": [
    {
      "type": "limit_order",
      "order_id": 1,
      "price_ticks": 99,
      "quantity": 100
    }
  ]
}
```

An empty decision is:

```json
{"type":"actions","actions":[]}
```

The evaluator validates actions using the same strict rules as built-in
strategies.

## Failure Rules

The current episode is invalidated with score zero when:

- the process exits before a required response;
- the response timeout expires;
- the response is not valid JSON;
- the response is not a JSON object;
- `type` is not `actions`;
- `actions` is not a list;
- an action violates the existing `execution_v1` rules.

The response timeout comes from `limits.local_callback_timeout_ms` in the
challenge file. It is a local reliability limit, not a security control.

Diagnostic logging belongs on stderr. Stdout is reserved for protocol lines.

## C++ Interface

[`arena/cpp/execution_v1_strategy.hpp`](../cpp/execution_v1_strategy.hpp)
provides `BookView`, `Portfolio`, `Action`, and `run_strategy_loop`. User
decision logic remains one function:

```cpp
std::vector<Action> onBookUpdate(
    const BookView& book,
    const Portfolio& portfolio);
```

The included adapter parses only this protocol. It is not a general JSON
library.
