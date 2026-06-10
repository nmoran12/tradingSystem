#!/usr/bin/env python3
"""Deterministic local evaluator skeleton for OrderBook Arena execution_v1."""

from __future__ import annotations

import argparse
import copy
import json
import sys
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any, Callable


MASK_64 = (1 << 64) - 1
RUNNER_VERSION = "execution_v1_evaluator_v1"
Strategy = Callable[[dict[str, Any], dict[str, Any]], list[dict[str, Any]]]


class ChallengeConfigError(ValueError):
    """Raised when an execution_v1 challenge cannot be evaluated."""


class SplitMix64:
    """Small explicit PRNG with stable unsigned 64-bit arithmetic."""

    def __init__(self, seed: int):
        if not isinstance(seed, int) or isinstance(seed, bool) or seed < 0:
            raise ValueError("seed must be a non-negative integer")
        self._state = seed & MASK_64

    def next_u64(self) -> int:
        self._state = (self._state + 0x9E3779B97F4A7C15) & MASK_64
        value = self._state
        value = ((value ^ (value >> 30)) * 0xBF58476D1CE4E5B9) & MASK_64
        value = ((value ^ (value >> 27)) * 0x94D049BB133111EB) & MASK_64
        return (value ^ (value >> 31)) & MASK_64

    def randbelow(self, upper_bound: int) -> int:
        if upper_bound <= 0:
            raise ValueError("upper_bound must be positive")
        return self.next_u64() % upper_bound

    def randint(self, lower_bound: int, upper_bound: int) -> int:
        if upper_bound < lower_bound:
            raise ValueError("upper_bound must not be below lower_bound")
        return lower_bound + self.randbelow(upper_bound - lower_bound + 1)

    def chance_bps(self, probability_bps: int) -> bool:
        return self.randbelow(10_000) < probability_bps


def _require_mapping(value: Any, field_name: str) -> dict[str, Any]:
    if not isinstance(value, dict):
        raise ChallengeConfigError(f"{field_name} must be an object")
    return value


def _require_positive_int(value: Any, field_name: str) -> int:
    if not isinstance(value, int) or isinstance(value, bool) or value <= 0:
        raise ChallengeConfigError(f"{field_name} must be a positive integer")
    return value


def _round_metric(value: float | None) -> float | None:
    return round(value, 6) if value is not None else None


def load_challenge(path: str | Path) -> dict[str, Any]:
    challenge_path = Path(path)
    try:
        with challenge_path.open(encoding="utf-8") as source:
            challenge = json.load(source)
    except (OSError, json.JSONDecodeError) as error:
        raise ChallengeConfigError(f"cannot load challenge: {error}") from error

    validate_challenge(challenge)
    return challenge


def validate_challenge(challenge: dict[str, Any]) -> None:
    if challenge.get("schema_version") != "1.0":
        raise ChallengeConfigError("schema_version must be 1.0")
    if challenge.get("challenge_type") != "execution_v1":
        raise ChallengeConfigError("challenge_type must be execution_v1")
    if challenge.get("challenge_id") != "beat_market_order":
        raise ChallengeConfigError("A3 supports only beat_market_order")
    if challenge.get("title") != "Beat the Market Order":
        raise ChallengeConfigError("challenge title must be Beat the Market Order")

    strategy = _require_mapping(challenge.get("strategy"), "strategy")
    if strategy.get("supported_languages") != ["python", "cpp"]:
        raise ChallengeConfigError("supported_languages must be ['python', 'cpp']")
    if strategy.get("callbacks") != {
        "python": "on_book_update",
        "cpp": "onBookUpdate",
    }:
        raise ChallengeConfigError("unsupported strategy callbacks")

    if challenge.get("allowed_actions") != [
        "market_order",
        "limit_order",
        "cancel_order",
    ]:
        raise ChallengeConfigError("unsupported allowed_actions")

    market = _require_mapping(challenge.get("market"), "market")
    generator = _require_mapping(
        market.get("scenario_generator"), "market.scenario_generator"
    )
    if generator.get("name") != "seeded_execution_market_v1":
        raise ChallengeConfigError("unsupported scenario generator")
    if generator.get("version") != 1:
        raise ChallengeConfigError("scenario generator version must be 1")
    if generator.get("prng") != {"name": "splitmix64", "version": 1}:
        raise ChallengeConfigError("scenario generator must use splitmix64 version 1")

    positive_generator_fields = (
        "event_count",
        "event_interval_ms",
        "initial_mid_price_ticks",
        "initial_spread_ticks",
        "initial_depth_levels",
        "level_spacing_ticks",
        "initial_quantity_per_level",
        "minimum_level_quantity",
        "maximum_level_quantity",
    )
    for field_name in positive_generator_fields:
        _require_positive_int(
            generator.get(field_name), f"market.scenario_generator.{field_name}"
        )

    for field_name in (
        "price_move_probability_bps",
        "liquidity_refresh_probability_bps",
    ):
        value = generator.get(field_name)
        if not isinstance(value, int) or isinstance(value, bool) or not 0 <= value <= 10_000:
            raise ChallengeConfigError(
                f"market.scenario_generator.{field_name} must be between 0 and 10000"
            )

    if generator["maximum_level_quantity"] < generator["minimum_level_quantity"]:
        raise ChallengeConfigError(
            "maximum_level_quantity must be at least minimum_level_quantity"
        )

    task = _require_mapping(challenge.get("task"), "task")
    if task.get("side") != "buy":
        raise ChallengeConfigError("execution_v1 task.side must be buy")
    target_quantity = _require_positive_int(
        task.get("target_quantity"), "task.target_quantity"
    )
    if task.get("start_event_index") != 0:
        raise ChallengeConfigError("task.start_event_index must be 0")
    if task.get("end_event_index") != generator["event_count"] - 1:
        raise ChallengeConfigError(
            "task.end_event_index must match scenario event_count"
        )
    if task.get("require_full_completion") is not True:
        raise ChallengeConfigError("execution_v1 requires full completion")

    initial_ask_liquidity = (
        generator["initial_depth_levels"] * generator["initial_quantity_per_level"]
    )
    if initial_ask_liquidity < target_quantity:
        raise ChallengeConfigError(
            "initial ask liquidity must cover the immediate-market baseline"
        )

    episodes = _require_mapping(challenge.get("episodes"), "episodes")
    all_seeds: list[int] = []
    for seed_set_name in ("public", "development", "local_evaluation"):
        seeds = episodes.get(seed_set_name)
        if not isinstance(seeds, list) or not seeds:
            raise ChallengeConfigError(
                f"episodes.{seed_set_name} must be a non-empty list"
            )
        for seed in seeds:
            if not isinstance(seed, int) or isinstance(seed, bool) or seed < 0:
                raise ChallengeConfigError(
                    f"episodes.{seed_set_name} seeds must be non-negative integers"
                )
        all_seeds.extend(seeds)
    if len(all_seeds) != len(set(all_seeds)):
        raise ChallengeConfigError("episode seeds must be unique across local sets")

    limits = _require_mapping(challenge.get("limits"), "limits")
    for field_name in (
        "max_orders_per_episode",
        "max_open_orders",
        "max_actions_per_event",
        "max_total_actions_per_episode",
        "max_order_quantity",
        "local_callback_timeout_ms",
    ):
        _require_positive_int(limits.get(field_name), f"limits.{field_name}")

    scoring = _require_mapping(challenge.get("scoring"), "scoring")
    if scoring.get("baseline_strategy") != "immediate_market_order_v1":
        raise ChallengeConfigError("unsupported baseline strategy")
    if scoring.get("completion_policy") != "full_target_required":
        raise ChallengeConfigError("unsupported completion policy")
    if scoring.get("completed_episode_score_metric") != "vwap_improvement_ticks":
        raise ChallengeConfigError("unsupported completed episode score metric")
    if scoring.get("incomplete_or_invalid_episode_score") != 0:
        raise ChallengeConfigError("invalid and incomplete episode score must be zero")
    if scoring.get("aggregate") != "arithmetic_mean":
        raise ChallengeConfigError("unsupported score aggregate")


@dataclass
class MarketState:
    config: dict[str, Any]
    rng: SplitMix64
    mid_price_ticks: int = field(init=False)
    bid_quantities: list[int] = field(init=False)
    ask_quantities: list[int] = field(init=False)

    def __post_init__(self) -> None:
        depth = self.config["initial_depth_levels"]
        quantity = self.config["initial_quantity_per_level"]
        self.mid_price_ticks = self.config["initial_mid_price_ticks"]
        self.bid_quantities = [quantity] * depth
        self.ask_quantities = [quantity] * depth

    def _best_prices(self) -> tuple[int, int]:
        spread = self.config["initial_spread_ticks"]
        best_bid = self.mid_price_ticks - spread // 2
        best_ask = best_bid + spread
        return best_bid, best_ask

    def snapshot(self) -> tuple[list[dict[str, int]], list[dict[str, int]]]:
        best_bid, best_ask = self._best_prices()
        spacing = self.config["level_spacing_ticks"]
        bids = [
            {
                "price_ticks": best_bid - index * spacing,
                "quantity": quantity,
            }
            for index, quantity in enumerate(self.bid_quantities)
            if quantity > 0
        ]
        asks = [
            {
                "price_ticks": best_ask + index * spacing,
                "quantity": quantity,
            }
            for index, quantity in enumerate(self.ask_quantities)
            if quantity > 0
        ]
        return bids, asks

    def advance(self) -> dict[str, Any]:
        previous_mid = self.mid_price_ticks
        if self.rng.chance_bps(self.config["price_move_probability_bps"]):
            self.mid_price_ticks += -1 if self.rng.randbelow(2) == 0 else 1

        refreshed_bid_levels: list[int] = []
        refreshed_ask_levels: list[int] = []
        minimum = self.config["minimum_level_quantity"]
        maximum = self.config["maximum_level_quantity"]
        probability = self.config["liquidity_refresh_probability_bps"]

        for index in range(len(self.bid_quantities)):
            if self.rng.chance_bps(probability):
                self.bid_quantities[index] = self.rng.randint(minimum, maximum)
                refreshed_bid_levels.append(index)
            if self.rng.chance_bps(probability):
                self.ask_quantities[index] = self.rng.randint(minimum, maximum)
                refreshed_ask_levels.append(index)

        return {
            "previous_mid_price_ticks": previous_mid,
            "mid_price_ticks": self.mid_price_ticks,
            "refreshed_bid_levels": refreshed_bid_levels,
            "refreshed_ask_levels": refreshed_ask_levels,
        }

    def execute_buy(
        self, quantity: int, limit_price_ticks: int | None = None
    ) -> tuple[list[dict[str, int]], int]:
        _, best_ask = self._best_prices()
        spacing = self.config["level_spacing_ticks"]
        remaining = quantity
        fills: list[dict[str, int]] = []

        for index, available in enumerate(self.ask_quantities):
            price = best_ask + index * spacing
            if limit_price_ticks is not None and price > limit_price_ticks:
                break
            if remaining == 0:
                break
            fill_quantity = min(available, remaining)
            if fill_quantity == 0:
                continue
            self.ask_quantities[index] -= fill_quantity
            remaining -= fill_quantity
            fills.append(
                {"price_ticks": price, "quantity": fill_quantity}
            )

        return fills, remaining


@dataclass
class EpisodeState:
    target_quantity: int
    market: MarketState
    filled_quantity: int = 0
    total_cost_tick_units: int = 0
    open_orders: dict[int, dict[str, int | str]] = field(default_factory=dict)
    used_order_ids: set[int] = field(default_factory=set)
    submitted_order_count: int = 0
    submitted_action_count: int = 0
    maximum_open_orders: int = 0
    invalid_reason: str | None = None

    @property
    def remaining_quantity(self) -> int:
        return self.target_quantity - self.filled_quantity

    def apply_fills(
        self,
        fills: list[dict[str, int]],
        replay: list[dict[str, Any]],
        event_index: int,
        order_id: int,
        source: str,
    ) -> None:
        for fill in fills:
            self.filled_quantity += fill["quantity"]
            self.total_cost_tick_units += fill["price_ticks"] * fill["quantity"]
            replay.append(
                {
                    "type": "fill",
                    "event_index": event_index,
                    "order_id": order_id,
                    "source": source,
                    **fill,
                }
            )

    def portfolio_view(self) -> dict[str, Any]:
        average_fill_price = (
            self.total_cost_tick_units / self.filled_quantity
            if self.filled_quantity
            else None
        )
        open_orders = [
            copy.deepcopy(order)
            for order in self.open_orders.values()
        ]
        return {
            "target_quantity": self.target_quantity,
            "filled_quantity": self.filled_quantity,
            "remaining_quantity": self.remaining_quantity,
            "total_cost_tick_units": self.total_cost_tick_units,
            "average_fill_price_ticks": _round_metric(average_fill_price),
            "open_orders": open_orders,
        }


def immediate_market_order_baseline(
    book: dict[str, Any], portfolio: dict[str, Any]
) -> list[dict[str, Any]]:
    if book["event_index"] != 0 or portfolio["remaining_quantity"] == 0:
        return []
    return [
        {
            "type": "market_order",
            "order_id": 1,
            "quantity": portfolio["remaining_quantity"],
        }
    ]


def simple_reference_limit_then_market_cleanup(
    book: dict[str, Any], portfolio: dict[str, Any]
) -> list[dict[str, Any]]:
    if portfolio["remaining_quantity"] == 0:
        return []

    if book["events_remaining"] == 0:
        actions = [
            {"type": "cancel_order", "order_id": order["order_id"]}
            for order in portfolio["open_orders"]
        ]
        actions.append(
            {
                "type": "market_order",
                "order_id": 1_000_000 + book["event_index"],
                "quantity": portfolio["remaining_quantity"],
            }
        )
        return actions

    if book["event_index"] == 0 and not portfolio["open_orders"]:
        return [
            {
                "type": "limit_order",
                "order_id": 1,
                "price_ticks": book["bids"][0]["price_ticks"] + 1,
                "quantity": portfolio["remaining_quantity"],
            }
        ]

    return []


BUILT_IN_STRATEGIES: dict[str, tuple[str, Strategy]] = {
    "baseline": (
        "immediate_market_order_baseline",
        immediate_market_order_baseline,
    ),
    "immediate_market_order_baseline": (
        "immediate_market_order_baseline",
        immediate_market_order_baseline,
    ),
    "simple_reference": (
        "simple_reference_limit_then_market_cleanup",
        simple_reference_limit_then_market_cleanup,
    ),
}


def _append_replay(
    replay: list[dict[str, Any]], record: dict[str, Any]
) -> None:
    replay.append(record)


def _invalidate(
    state: EpisodeState,
    replay: list[dict[str, Any]],
    event_index: int,
    reason: str,
) -> None:
    state.invalid_reason = reason
    _append_replay(
        replay,
        {"type": "episode_invalid", "event_index": event_index, "reason": reason},
    )


def _validate_new_order(
    action: dict[str, Any],
    state: EpisodeState,
    limits: dict[str, Any],
) -> tuple[int, int] | str:
    order_id = action.get("order_id")
    quantity = action.get("quantity")
    if not isinstance(order_id, int) or isinstance(order_id, bool) or order_id <= 0:
        return "order_id must be a positive integer"
    if order_id in state.used_order_ids:
        return f"order_id {order_id} has already been used"
    if not isinstance(quantity, int) or isinstance(quantity, bool) or quantity <= 0:
        return "quantity must be a positive integer"
    if quantity > limits["max_order_quantity"]:
        return "quantity exceeds max_order_quantity"
    reserved_quantity = sum(
        int(order["remaining_quantity"]) for order in state.open_orders.values()
    )
    if quantity > state.remaining_quantity - reserved_quantity:
        return "quantity exceeds uncommitted remaining target"
    if state.submitted_order_count >= limits["max_orders_per_episode"]:
        return "max_orders_per_episode exceeded"
    return order_id, quantity


def _apply_action(
    action: Any,
    state: EpisodeState,
    limits: dict[str, Any],
    replay: list[dict[str, Any]],
    event_index: int,
) -> str | None:
    if not isinstance(action, dict):
        return "action must be an object"
    action_type = action.get("type")
    _append_replay(
        replay,
        {
            "type": "strategy_action",
            "event_index": event_index,
            "action": copy.deepcopy(action),
        },
    )

    if action_type == "cancel_order":
        order_id = action.get("order_id")
        if not isinstance(order_id, int) or isinstance(order_id, bool):
            return "cancel order_id must be an integer"
        if order_id not in state.open_orders:
            return f"cannot cancel unknown open order {order_id}"
        cancelled = state.open_orders.pop(order_id)
        _append_replay(
            replay,
            {
                "type": "order_cancelled",
                "event_index": event_index,
                "order": cancelled,
            },
        )
        return None

    if action_type not in ("market_order", "limit_order"):
        return f"unsupported action type: {action_type!r}"

    validated = _validate_new_order(action, state, limits)
    if isinstance(validated, str):
        return validated
    order_id, quantity = validated
    state.used_order_ids.add(order_id)
    state.submitted_order_count += 1

    if action_type == "market_order":
        fills, unfilled = state.market.execute_buy(quantity)
        state.apply_fills(fills, replay, event_index, order_id, "market_order")
        _append_replay(
            replay,
            {
                "type": "order_processed",
                "event_index": event_index,
                "order_id": order_id,
                "order_type": action_type,
                "requested_quantity": quantity,
                "unfilled_quantity": unfilled,
            },
        )
        return None

    price = action.get("price_ticks")
    if not isinstance(price, int) or isinstance(price, bool) or price <= 0:
        return "limit price_ticks must be a positive integer"
    fills, unfilled = state.market.execute_buy(quantity, price)
    state.apply_fills(fills, replay, event_index, order_id, "limit_order")
    if unfilled:
        if len(state.open_orders) >= limits["max_open_orders"]:
            return "max_open_orders exceeded"
        state.open_orders[order_id] = {
            "order_id": order_id,
            "type": "limit_order",
            "price_ticks": price,
            "remaining_quantity": unfilled,
        }
        state.maximum_open_orders = max(
            state.maximum_open_orders, len(state.open_orders)
        )
    _append_replay(
        replay,
        {
            "type": "order_processed",
            "event_index": event_index,
            "order_id": order_id,
            "order_type": action_type,
            "price_ticks": price,
            "requested_quantity": quantity,
            "resting_quantity": unfilled,
        },
    )
    return None


def _fill_marketable_open_orders(
    state: EpisodeState,
    replay: list[dict[str, Any]],
    event_index: int,
) -> None:
    for order_id in list(state.open_orders):
        order = state.open_orders[order_id]
        fills, unfilled = state.market.execute_buy(
            int(order["remaining_quantity"]), int(order["price_ticks"])
        )
        state.apply_fills(fills, replay, event_index, order_id, "resting_limit")
        if unfilled:
            order["remaining_quantity"] = unfilled
        else:
            del state.open_orders[order_id]
            _append_replay(
                replay,
                {
                    "type": "order_filled",
                    "event_index": event_index,
                    "order_id": order_id,
                },
            )


def _book_view(
    challenge: dict[str, Any],
    state: EpisodeState,
    event_index: int,
) -> dict[str, Any]:
    generator = challenge["market"]["scenario_generator"]
    bids, asks = state.market.snapshot()
    visible_depth = challenge["strategy"]["book_view"]["visible_depth_levels"]
    return {
        "event_index": event_index,
        "events_remaining": generator["event_count"] - event_index - 1,
        "timestamp_ms": event_index * generator["event_interval_ms"],
        "symbol": challenge["market"]["symbol"],
        "bids": bids[:visible_depth],
        "asks": asks[:visible_depth],
    }


def _episode_metrics(
    challenge: dict[str, Any],
    state: EpisodeState,
    seed: int,
    arrival_midpoint_ticks: float,
    baseline_vwap_ticks: float | None,
) -> dict[str, Any]:
    completed = state.invalid_reason is None and state.remaining_quantity == 0
    if state.invalid_reason is not None:
        status = "invalid"
        error_reason = state.invalid_reason
    elif not completed:
        status = "incomplete"
        error_reason = "target quantity not completed"
    else:
        status = "completed"
        error_reason = None

    strategy_vwap = (
        state.total_cost_tick_units / state.filled_quantity
        if state.filled_quantity
        else None
    )
    strategy_slippage = (
        strategy_vwap - arrival_midpoint_ticks
        if strategy_vwap is not None
        else None
    )
    baseline_slippage = (
        baseline_vwap_ticks - arrival_midpoint_ticks
        if baseline_vwap_ticks is not None
        else None
    )
    improvement = (
        baseline_vwap_ticks - strategy_vwap
        if completed
        and strategy_vwap is not None
        and baseline_vwap_ticks is not None
        else None
    )
    score = (
        improvement
        if improvement is not None
        else challenge["scoring"]["incomplete_or_invalid_episode_score"]
    )

    return {
        "seed": seed,
        "status": status,
        "completed": completed,
        "score": _round_metric(score),
        "completion_ratio": _round_metric(
            state.filled_quantity / state.target_quantity
        ),
        "filled_quantity": state.filled_quantity,
        "remaining_quantity": state.remaining_quantity,
        "strategy_vwap_ticks": _round_metric(strategy_vwap),
        "baseline_vwap_ticks": _round_metric(baseline_vwap_ticks),
        "arrival_midpoint_ticks": _round_metric(arrival_midpoint_ticks),
        "strategy_slippage_ticks": _round_metric(strategy_slippage),
        "baseline_slippage_ticks": _round_metric(baseline_slippage),
        "execution_cost_improvement_ticks": _round_metric(improvement),
        "submitted_order_count": state.submitted_order_count,
        "submitted_action_count": state.submitted_action_count,
        "maximum_open_orders": state.maximum_open_orders,
        "error_reason": error_reason,
    }


def evaluate_episode(
    challenge: dict[str, Any],
    seed: int,
    strategy: Strategy,
    strategy_name: str,
    baseline_vwap_ticks: float | None = None,
) -> tuple[dict[str, Any], list[dict[str, Any]]]:
    generator = challenge["market"]["scenario_generator"]
    limits = challenge["limits"]
    market = MarketState(generator, SplitMix64(seed))
    state = EpisodeState(challenge["task"]["target_quantity"], market)
    replay: list[dict[str, Any]] = [
        {
            "type": "episode_start",
            "runner_version": RUNNER_VERSION,
            "challenge_id": challenge["challenge_id"],
            "challenge_type": challenge["challenge_type"],
            "strategy": strategy_name,
            "seed": seed,
            "generator": {
                "name": generator["name"],
                "version": generator["version"],
                "prng": copy.deepcopy(generator["prng"]),
            },
            "simulation_model": "python_level_book_skeleton_v1",
            "uses_cpp_matching_engine": False,
        }
    ]

    initial_bids, initial_asks = market.snapshot()
    arrival_midpoint_ticks = (
        initial_bids[0]["price_ticks"] + initial_asks[0]["price_ticks"]
    ) / 2

    for event_index in range(generator["event_count"]):
        if event_index > 0:
            market_update = market.advance()
            _append_replay(
                replay,
                {
                    "type": "market_update",
                    "event_index": event_index,
                    **market_update,
                },
            )
            _fill_marketable_open_orders(state, replay, event_index)

        if state.remaining_quantity == 0:
            break

        book = _book_view(challenge, state, event_index)
        portfolio = state.portfolio_view()
        _append_replay(
            replay,
            {
                "type": "book_update",
                "event_index": event_index,
                "book": copy.deepcopy(book),
                "portfolio": copy.deepcopy(portfolio),
            },
        )

        try:
            actions = strategy(copy.deepcopy(book), copy.deepcopy(portfolio))
        except Exception as error:  # Built-ins today; external adapters come later.
            _invalidate(
                state,
                replay,
                event_index,
                f"strategy callback raised {type(error).__name__}: {error}",
            )
            break

        if not isinstance(actions, list):
            _invalidate(
                state, replay, event_index, "strategy callback must return a list"
            )
            break
        if len(actions) > limits["max_actions_per_event"]:
            _invalidate(
                state, replay, event_index, "max_actions_per_event exceeded"
            )
            break
        if state.submitted_action_count + len(actions) > limits[
            "max_total_actions_per_episode"
        ]:
            _invalidate(
                state,
                replay,
                event_index,
                "max_total_actions_per_episode exceeded",
            )
            break

        state.submitted_action_count += len(actions)
        for action in actions:
            error_reason = _apply_action(
                action, state, limits, replay, event_index
            )
            if error_reason is not None:
                _invalidate(state, replay, event_index, error_reason)
                break
        if state.invalid_reason is not None:
            break

        _append_replay(
            replay,
            {
                "type": "portfolio_update",
                "event_index": event_index,
                "portfolio": state.portfolio_view(),
            },
        )

    if baseline_vwap_ticks is None and strategy_name == "immediate_market_order_baseline":
        baseline_vwap_ticks = (
            state.total_cost_tick_units / state.filled_quantity
            if state.filled_quantity
            else None
        )

    metrics = _episode_metrics(
        challenge,
        state,
        seed,
        arrival_midpoint_ticks,
        baseline_vwap_ticks,
    )
    _append_replay(replay, {"type": "episode_result", **copy.deepcopy(metrics)})

    for sequence, record in enumerate(replay):
        record["sequence"] = sequence
        record["seed"] = seed

    return metrics, replay


def _baseline_for_seed(
    challenge: dict[str, Any], seed: int
) -> tuple[dict[str, Any], list[dict[str, Any]]]:
    return evaluate_episode(
        challenge,
        seed,
        immediate_market_order_baseline,
        "immediate_market_order_baseline",
    )


def evaluate_challenge(
    challenge: dict[str, Any],
    strategy: Strategy,
    strategy_name: str,
    seed_set: str = "local_evaluation",
) -> tuple[dict[str, Any], list[dict[str, Any]]]:
    seeds = challenge["episodes"].get(seed_set)
    if not isinstance(seeds, list) or not seeds:
        raise ChallengeConfigError(f"unknown or empty seed set: {seed_set}")

    episode_results: list[dict[str, Any]] = []
    replay_records: list[dict[str, Any]] = []

    for seed in seeds:
        baseline_result, _ = _baseline_for_seed(challenge, seed)
        if not baseline_result["completed"]:
            raise RuntimeError(f"baseline did not complete for seed {seed}")

        episode_result, episode_replay = evaluate_episode(
            challenge,
            seed,
            strategy,
            strategy_name,
            baseline_result["strategy_vwap_ticks"],
        )
        episode_results.append(episode_result)
        replay_records.extend(episode_replay)

    aggregate_score = sum(result["score"] for result in episode_results) / len(
        episode_results
    )
    completed_count = sum(result["completed"] for result in episode_results)
    invalid_count = sum(
        result["status"] == "invalid" for result in episode_results
    )

    result = {
        "result_schema_version": challenge["outputs"]["result"]["schema_version"],
        "runner_version": RUNNER_VERSION,
        "challenge": {
            "schema_version": challenge["schema_version"],
            "challenge_id": challenge["challenge_id"],
            "title": challenge["title"],
            "challenge_type": challenge["challenge_type"],
        },
        "strategy": {
            "name": strategy_name,
            "kind": "built_in",
            "api_version": challenge["strategy"]["api_version"],
        },
        "simulation": {
            "model": "python_level_book_skeleton_v1",
            "uses_cpp_matching_engine": False,
            "configured_engine_version": challenge["market"]["engine_version"],
            "scenario_generator": {
                "name": challenge["market"]["scenario_generator"]["name"],
                "version": challenge["market"]["scenario_generator"]["version"],
                "prng": copy.deepcopy(
                    challenge["market"]["scenario_generator"]["prng"]
                ),
            },
        },
        "evaluation": {
            "seed_set": seed_set,
            "seeds": list(seeds),
            "episode_count": len(episode_results),
            "completed_episode_count": completed_count,
            "invalid_episode_count": invalid_count,
            "all_completed": completed_count == len(episode_results),
        },
        "scoring": {
            "version": challenge["scoring"]["version"],
            "metric": challenge["scoring"]["completed_episode_score_metric"],
            "aggregate": challenge["scoring"]["aggregate"],
            "aggregate_score": _round_metric(aggregate_score),
            "higher_is_better": challenge["scoring"]["higher_is_better"],
            "baseline_strategy": challenge["scoring"]["baseline_strategy"],
        },
        "episodes": episode_results,
        "replay": {
            "format": challenge["outputs"]["replay"]["format"],
            "schema_version": challenge["outputs"]["replay"]["schema_version"],
        },
    }
    return result, replay_records


def write_result(path: str | Path, result: dict[str, Any]) -> None:
    output_path = Path(path)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    with output_path.open("w", encoding="utf-8") as output:
        json.dump(result, output, indent=2, sort_keys=True)
        output.write("\n")


def write_replay(path: str | Path, records: list[dict[str, Any]]) -> None:
    output_path = Path(path)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    with output_path.open("w", encoding="utf-8") as output:
        for record in records:
            output.write(json.dumps(record, sort_keys=True, separators=(",", ":")))
            output.write("\n")


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Evaluate a built-in strategy for an execution_v1 challenge."
    )
    parser.add_argument("--challenge", required=True, help="Challenge JSON path")
    parser.add_argument(
        "--strategy",
        required=True,
        choices=sorted(BUILT_IN_STRATEGIES),
        help="Built-in evaluator strategy",
    )
    parser.add_argument(
        "--seed-set",
        default="local_evaluation",
        choices=("public", "development", "local_evaluation"),
    )
    parser.add_argument("--results-out", required=True, help="Result JSON path")
    parser.add_argument("--replay-out", required=True, help="Replay JSONL path")
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    args = parse_args(sys.argv[1:] if argv is None else argv)
    try:
        challenge = load_challenge(args.challenge)
        strategy_name, strategy = BUILT_IN_STRATEGIES[args.strategy]
        result, replay = evaluate_challenge(
            challenge, strategy, strategy_name, args.seed_set
        )
        result["replay"]["path"] = args.replay_out
        write_result(args.results_out, result)
        write_replay(args.replay_out, replay)
    except (ChallengeConfigError, OSError, RuntimeError) as error:
        print(f"evaluation failed: {error}", file=sys.stderr)
        return 1

    print(
        json.dumps(
            {
                "aggregate_score": result["scoring"]["aggregate_score"],
                "all_completed": result["evaluation"]["all_completed"],
                "results_out": args.results_out,
                "replay_out": args.replay_out,
            },
            sort_keys=True,
        )
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
