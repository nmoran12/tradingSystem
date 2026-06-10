#!/usr/bin/env python3
"""Standalone local execution_v1 Python strategy example."""

from __future__ import annotations

import json
import sys
from typing import Any


def on_book_update(
    book_update: dict[str, Any],
    portfolio: dict[str, Any],
) -> list[dict[str, Any]]:
    if portfolio["remaining_quantity"] == 0:
        return []

    if book_update["events_remaining"] == 0:
        actions = [
            {"type": "cancel_order", "order_id": order["order_id"]}
            for order in portfolio["open_orders"]
        ]
        actions.append(
            {
                "type": "market_order",
                "order_id": 1_000_000 + book_update["event_index"],
                "quantity": portfolio["remaining_quantity"],
            }
        )
        return actions

    if book_update["event_index"] == 0 and not portfolio["open_orders"]:
        return [
            {
                "type": "limit_order",
                "order_id": 1,
                "price_ticks": book_update["book"]["bids"][0]["price_ticks"] + 1,
                "quantity": portfolio["remaining_quantity"],
            }
        ]

    return []


def _write_response(actions: list[dict[str, Any]]) -> None:
    sys.stdout.write(
        json.dumps({"type": "actions", "actions": actions}, separators=(",", ":"))
    )
    sys.stdout.write("\n")
    sys.stdout.flush()


def main() -> int:
    for raw_line in sys.stdin:
        line = raw_line.strip()
        if not line:
            continue

        message = json.loads(line)
        message_type = message.get("type")
        if message_type == "book_update":
            _write_response(on_book_update(message, message["portfolio"]))
        elif message_type == "episode_end":
            continue
        elif message_type == "evaluation_end":
            break
        else:
            raise ValueError(f"unsupported protocol message: {message_type!r}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
