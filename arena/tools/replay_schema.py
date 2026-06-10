"""Replay schema constants and validation for OrderBook Arena JSONL."""

from __future__ import annotations

from typing import Any


REPLAY_SCHEMA_VERSION = "1.0"


class ReplaySchemaError(ValueError):
    """Raised when replay records do not satisfy the Arena replay contract."""


def _require_mapping(value: Any, field_name: str) -> dict[str, Any]:
    if not isinstance(value, dict):
        raise ReplaySchemaError(f"{field_name} must be an object")
    return value


def _require_int(value: Any, field_name: str) -> int:
    if not isinstance(value, int) or isinstance(value, bool):
        raise ReplaySchemaError(f"{field_name} must be an integer")
    return value


def validate_replay_records(records: list[dict[str, Any]]) -> None:
    if not records:
        raise ReplaySchemaError("replay must contain at least one record")

    seeds: list[int] = []
    expected_episode_sequence: dict[int, int] = {}
    episode_types: dict[int, list[str]] = {}

    for record_index, record in enumerate(records):
        if not isinstance(record, dict):
            raise ReplaySchemaError(
                f"record {record_index} must be an object"
            )
        if record.get("replay_schema_version") != REPLAY_SCHEMA_VERSION:
            raise ReplaySchemaError(
                f"record {record_index} has unsupported replay schema version"
            )
        if record.get("record_index") != record_index:
            raise ReplaySchemaError(
                f"record {record_index} has non-contiguous record_index"
            )

        record_type = record.get("type")
        if not isinstance(record_type, str) or not record_type:
            raise ReplaySchemaError(
                f"record {record_index}.type must be a non-empty string"
            )
        seed = _require_int(record.get("seed"), f"record {record_index}.seed")
        episode_sequence = _require_int(
            record.get("episode_sequence"),
            f"record {record_index}.episode_sequence",
        )
        expected = expected_episode_sequence.get(seed, 0)
        if episode_sequence != expected:
            raise ReplaySchemaError(
                f"record {record_index} has non-contiguous episode_sequence"
            )
        expected_episode_sequence[seed] = expected + 1
        episode_types.setdefault(seed, []).append(record_type)
        if seed not in seeds:
            seeds.append(seed)

        event_index = record.get("event_index")
        if event_index is not None:
            _require_int(event_index, f"record {record_index}.event_index")

        if record_type == "book_update":
            book = _require_mapping(
                record.get("book"), f"record {record_index}.book"
            )
            if not isinstance(book.get("bids"), list) or not isinstance(
                book.get("asks"), list
            ):
                raise ReplaySchemaError(
                    f"record {record_index}.book requires bid and ask lists"
                )
            _require_mapping(
                record.get("portfolio"),
                f"record {record_index}.portfolio",
            )
        elif record_type == "strategy_action":
            if "action" not in record:
                raise ReplaySchemaError(
                    f"record {record_index}.action is required"
                )
        elif record_type == "action_result":
            if record.get("status") not in ("accepted", "rejected"):
                raise ReplaySchemaError(
                    f"record {record_index}.status must be accepted or rejected"
                )
            if "action" not in record:
                raise ReplaySchemaError(
                    f"record {record_index}.action is required"
                )
        elif record_type == "fill":
            _require_int(
                record.get("price_ticks"),
                f"record {record_index}.price_ticks",
            )
            _require_int(
                record.get("quantity"), f"record {record_index}.quantity"
            )
        elif record_type == "portfolio_update":
            _require_mapping(
                record.get("portfolio"),
                f"record {record_index}.portfolio",
            )
        elif record_type == "episode_result":
            if record.get("status") not in (
                "completed",
                "incomplete",
                "invalid",
            ):
                raise ReplaySchemaError(
                    f"record {record_index} has invalid episode status"
                )
            _require_mapping(
                record.get("book"), f"record {record_index}.book"
            )
            _require_mapping(
                record.get("portfolio"),
                f"record {record_index}.portfolio",
            )

    for seed in seeds:
        record_types = episode_types[seed]
        if record_types[0] != "episode_start":
            raise ReplaySchemaError(
                f"episode {seed} must start with episode_start"
            )
        if record_types[-1] != "episode_result":
            raise ReplaySchemaError(
                f"episode {seed} must end with episode_result"
            )
