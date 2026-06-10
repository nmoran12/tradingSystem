"""Versioned execution_v1 scoring metrics and result document assembly."""

from __future__ import annotations

import copy
from typing import Any


RESULT_SCHEMA_VERSION = "1.1"
RESULT_GENERATOR_NAME = "arena_execution_result"
RESULT_GENERATOR_VERSION = "1.0"
TIMESTAMP_POLICY = "none"
SIMULATOR_MODEL = "python_level_book_skeleton_v1"


def round_metric(value: float | None) -> float | None:
    return round(value, 6) if value is not None else None


def calculate_episode_metrics(
    *,
    challenge: dict[str, Any],
    seed: int,
    target_quantity: int,
    filled_quantity: int,
    cash_spent_ticks: int,
    baseline_average_fill_price_ticks: float | None,
    arrival_midpoint_ticks: float,
    invalid_reason: str | None,
    events_processed: int,
    actions_submitted: int,
    orders_submitted: int,
    fill_count: int,
    maximum_open_orders: int,
) -> dict[str, Any]:
    valid = invalid_reason is None
    remaining_quantity = target_quantity - filled_quantity
    completed = valid and remaining_quantity == 0

    if not valid:
        status = "invalid"
        error_reason = invalid_reason
        error = {"code": "invalid_episode", "message": invalid_reason}
    elif not completed:
        status = "incomplete"
        error_reason = "target quantity not completed"
        error = {
            "code": "incomplete_target",
            "message": error_reason,
        }
    else:
        status = "completed"
        error_reason = None
        error = None

    average_fill_price_ticks = (
        cash_spent_ticks / filled_quantity if filled_quantity else None
    )
    fill_rate = filled_quantity / target_quantity
    slippage_ticks = (
        average_fill_price_ticks - arrival_midpoint_ticks
        if average_fill_price_ticks is not None
        else None
    )
    slippage_bps = (
        slippage_ticks / arrival_midpoint_ticks * 10_000
        if slippage_ticks is not None and arrival_midpoint_ticks != 0
        else None
    )
    baseline_slippage_ticks = (
        baseline_average_fill_price_ticks - arrival_midpoint_ticks
        if baseline_average_fill_price_ticks is not None
        else None
    )
    baseline_slippage_bps = (
        baseline_slippage_ticks / arrival_midpoint_ticks * 10_000
        if baseline_slippage_ticks is not None and arrival_midpoint_ticks != 0
        else None
    )
    improvement_ticks = (
        baseline_average_fill_price_ticks - average_fill_price_ticks
        if completed
        and baseline_average_fill_price_ticks is not None
        and average_fill_price_ticks is not None
        else None
    )
    improvement_bps = (
        improvement_ticks / baseline_average_fill_price_ticks * 10_000
        if improvement_ticks is not None
        and baseline_average_fill_price_ticks not in (None, 0)
        else None
    )
    score = (
        improvement_ticks
        if improvement_ticks is not None
        else challenge["scoring"]["incomplete_or_invalid_episode_score"]
    )

    generator = challenge["market"]["scenario_generator"]
    return {
        "seed": seed,
        "status": status,
        "valid": valid,
        "completed": completed,
        "score": round_metric(score),
        "target_quantity": target_quantity,
        "filled_quantity": filled_quantity,
        "remaining_quantity": remaining_quantity,
        "fill_rate": round_metric(fill_rate),
        "cash_spent_ticks": cash_spent_ticks,
        "average_fill_price_ticks": round_metric(average_fill_price_ticks),
        "baseline_average_fill_price_ticks": round_metric(
            baseline_average_fill_price_ticks
        ),
        "arrival_midpoint_ticks": round_metric(arrival_midpoint_ticks),
        "slippage_ticks": round_metric(slippage_ticks),
        "slippage_bps": round_metric(slippage_bps),
        "baseline_slippage_ticks": round_metric(baseline_slippage_ticks),
        "baseline_slippage_bps": round_metric(baseline_slippage_bps),
        "improvement_ticks": round_metric(improvement_ticks),
        "improvement_bps": round_metric(improvement_bps),
        "events_processed": events_processed,
        "actions_submitted": actions_submitted,
        "orders_submitted": orders_submitted,
        "fill_count": fill_count,
        "maximum_open_orders": maximum_open_orders,
        "invalid_reason": invalid_reason,
        "error": error,
        # Compatibility aliases retained for A3/A4 replay consumers.
        "completion_ratio": round_metric(fill_rate),
        "strategy_vwap_ticks": round_metric(average_fill_price_ticks),
        "baseline_vwap_ticks": round_metric(
            baseline_average_fill_price_ticks
        ),
        "strategy_slippage_ticks": round_metric(slippage_ticks),
        "execution_cost_improvement_ticks": round_metric(improvement_ticks),
        "submitted_action_count": actions_submitted,
        "submitted_order_count": orders_submitted,
        "error_reason": error_reason,
        "reproduction": {
            "challenge_id": challenge["challenge_id"],
            "challenge_version": challenge["challenge_version"],
            "challenge_schema_version": challenge["schema_version"],
            "simulator_model": SIMULATOR_MODEL,
            "score_version": challenge["scoring"]["version"],
            "scenario_generator": {
                "name": generator["name"],
                "version": generator["version"],
                "prng": copy.deepcopy(generator["prng"]),
            },
            "seed": seed,
        },
    }


def build_result_document(
    *,
    challenge: dict[str, Any],
    challenge_source: str,
    strategy_name: str,
    strategy_mode: str,
    runner_version: str,
    seed_set: str,
    seeds: list[int],
    episode_results: list[dict[str, Any]],
    replay_record_count: int,
) -> dict[str, Any]:
    episode_count = len(episode_results)
    completed_count = sum(result["completed"] for result in episode_results)
    valid_count = sum(result["valid"] for result in episode_results)
    invalid_count = episode_count - valid_count
    incomplete_count = sum(
        result["valid"] and not result["completed"]
        for result in episode_results
    )
    aggregate_score = (
        sum(result["score"] for result in episode_results) / episode_count
    )
    mean_fill_rate = (
        sum(result["fill_rate"] for result in episode_results) / episode_count
    )
    completed_improvements = [
        result["improvement_ticks"]
        for result in episode_results
        if result["improvement_ticks"] is not None
    ]
    mean_completed_improvement = (
        sum(completed_improvements) / len(completed_improvements)
        if completed_improvements
        else None
    )
    generator = challenge["market"]["scenario_generator"]

    return {
        "schema_version": RESULT_SCHEMA_VERSION,
        "result_schema_version": RESULT_SCHEMA_VERSION,
        "result_generator": {
            "name": RESULT_GENERATOR_NAME,
            "version": RESULT_GENERATOR_VERSION,
            "timestamp_policy": TIMESTAMP_POLICY,
        },
        "runner_version": runner_version,
        "challenge": {
            "id": challenge["challenge_id"],
            "challenge_id": challenge["challenge_id"],
            "version": challenge["challenge_version"],
            "schema_version": challenge["schema_version"],
            "type": challenge["challenge_type"],
            "challenge_type": challenge["challenge_type"],
            "title": challenge["title"],
            "source": challenge_source,
        },
        "strategy": {
            "mode": strategy_mode,
            "identifier": strategy_name,
            "name": strategy_name,
            "kind": strategy_mode,
            "api_version": challenge["strategy"]["api_version"],
        },
        "simulation": {
            "model": SIMULATOR_MODEL,
            "uses_cpp_matching_engine": False,
            "configured_engine_version": challenge["market"]["engine_version"],
            "scenario_generator": {
                "name": generator["name"],
                "version": generator["version"],
                "prng": copy.deepcopy(generator["prng"]),
            },
        },
        "reproduction": {
            "deterministic": True,
            "timestamp_policy": TIMESTAMP_POLICY,
            "challenge_source": challenge_source,
            "seed_set": seed_set,
            "seeds": list(seeds),
            "runner_version": runner_version,
            "result_generator_version": RESULT_GENERATOR_VERSION,
            "score_version": challenge["scoring"]["version"],
            "simulator_model": SIMULATOR_MODEL,
        },
        "evaluation": {
            "seed_set": seed_set,
            "seeds": list(seeds),
            "episode_count": episode_count,
            "valid_episode_count": valid_count,
            "completed_episode_count": completed_count,
            "incomplete_episode_count": incomplete_count,
            "invalid_episode_count": invalid_count,
            "all_valid": valid_count == episode_count,
            "all_completed": completed_count == episode_count,
        },
        "scoring": {
            "version": challenge["scoring"]["version"],
            "score_version": challenge["scoring"]["version"],
            "metric": challenge["scoring"]["completed_episode_score_metric"],
            "episode_formula": (
                "baseline_average_fill_price_ticks"
                " - average_fill_price_ticks"
            ),
            "invalid_episode_score": (
                challenge["scoring"]["incomplete_or_invalid_episode_score"]
            ),
            "incomplete_episode_score": (
                challenge["scoring"]["incomplete_or_invalid_episode_score"]
            ),
            "aggregate": challenge["scoring"]["aggregate"],
            "aggregate_formula": "mean(all episode scores)",
            "aggregate_score": round_metric(aggregate_score),
            "higher_is_better": challenge["scoring"]["higher_is_better"],
            "baseline_strategy": challenge["scoring"]["baseline_strategy"],
        },
        "aggregate_metrics": {
            "episode_count": episode_count,
            "valid_episode_count": valid_count,
            "completed_episode_count": completed_count,
            "incomplete_episode_count": incomplete_count,
            "invalid_episode_count": invalid_count,
            "mean_fill_rate": round_metric(mean_fill_rate),
            "mean_completed_improvement_ticks": round_metric(
                mean_completed_improvement
            ),
            "score": round_metric(aggregate_score),
        },
        "episodes": episode_results,
        "replay": {
            "format": challenge["outputs"]["replay"]["format"],
            "schema_version": challenge["outputs"]["replay"]["schema_version"],
            "artifact_name": challenge["outputs"]["replay"][
                "default_filename"
            ],
            "record_count": replay_record_count,
            "seeds": list(seeds),
        },
    }
