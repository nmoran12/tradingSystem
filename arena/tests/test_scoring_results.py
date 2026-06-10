import json
import tempfile
import unittest
from pathlib import Path

from arena.tools.evaluate_execution_v1 import (
    evaluate_challenge,
    load_challenge,
    simple_reference_limit_then_market_cleanup,
    write_result,
)
from arena.tools.scoring_results import calculate_episode_metrics


ROOT = Path(__file__).resolve().parents[2]
CHALLENGE_PATH = ROOT / "arena/challenges/beat_market_order.v1.json"
RESULT_FIXTURES = ROOT / "arena/tests/fixtures/results"
EPISODE_GOLDEN_FIELDS = (
    "seed",
    "status",
    "valid",
    "completed",
    "score",
    "filled_quantity",
    "fill_rate",
    "cash_spent_ticks",
    "average_fill_price_ticks",
    "slippage_ticks",
    "slippage_bps",
    "improvement_ticks",
    "improvement_bps",
    "events_processed",
    "fill_count",
    "invalid_reason",
)


def result_golden_projection(result):
    return {
        "schema_version": result["schema_version"],
        "result_generator": result["result_generator"],
        "strategy": {
            "mode": result["strategy"]["mode"],
            "identifier": result["strategy"]["identifier"],
        },
        "score_version": result["scoring"]["score_version"],
        "aggregate_metrics": result["aggregate_metrics"],
        "episodes": [
            {
                field: episode[field]
                for field in EPISODE_GOLDEN_FIELDS
                if field in episode
            }
            for episode in result["episodes"]
        ],
    }


class ScoringResultsTest(unittest.TestCase):
    def setUp(self):
        self.challenge = load_challenge(CHALLENGE_PATH)

    def calculate(self, **overrides):
        arguments = {
            "challenge": self.challenge,
            "seed": 123,
            "target_quantity": 100,
            "filled_quantity": 100,
            "cash_spent_ticks": 10_050,
            "baseline_average_fill_price_ticks": 101.0,
            "arrival_midpoint_ticks": 100.0,
            "invalid_reason": None,
            "events_processed": 4,
            "actions_submitted": 2,
            "orders_submitted": 2,
            "fill_count": 2,
            "maximum_open_orders": 1,
        }
        arguments.update(overrides)
        return calculate_episode_metrics(**arguments)

    def assert_golden(self, result, fixture_name):
        expected = json.loads(
            (RESULT_FIXTURES / fixture_name).read_text(encoding="utf-8")
        )
        self.assertEqual(result_golden_projection(result), expected)

    def test_completed_fill_metrics_are_hand_calculated(self):
        result = self.calculate()

        self.assertEqual(result["average_fill_price_ticks"], 100.5)
        self.assertEqual(result["baseline_average_fill_price_ticks"], 101.0)
        self.assertEqual(result["fill_rate"], 1.0)
        self.assertEqual(result["slippage_ticks"], 0.5)
        self.assertEqual(result["slippage_bps"], 50.0)
        self.assertEqual(result["improvement_ticks"], 0.5)
        self.assertEqual(result["improvement_bps"], 49.50495)
        self.assertEqual(result["score"], 0.5)
        self.assertTrue(result["completed"])
        self.assertTrue(result["valid"])

    def test_zero_fill_metrics_are_explicit(self):
        result = self.calculate(
            filled_quantity=0,
            cash_spent_ticks=0,
            fill_count=0,
        )

        self.assertEqual(result["fill_rate"], 0.0)
        self.assertEqual(result["remaining_quantity"], 100)
        self.assertIsNone(result["average_fill_price_ticks"])
        self.assertIsNone(result["slippage_ticks"])
        self.assertIsNone(result["improvement_ticks"])
        self.assertEqual(result["score"], 0)
        self.assertFalse(result["completed"])
        self.assertTrue(result["valid"])

    def test_partial_fill_is_incomplete_and_scores_zero(self):
        result = self.calculate(
            filled_quantity=50,
            cash_spent_ticks=5_000,
            fill_count=1,
        )

        self.assertEqual(result["average_fill_price_ticks"], 100.0)
        self.assertEqual(result["fill_rate"], 0.5)
        self.assertEqual(result["remaining_quantity"], 50)
        self.assertIsNone(result["improvement_ticks"])
        self.assertEqual(result["score"], 0)
        self.assertEqual(result["status"], "incomplete")

    def test_completed_underperformance_can_score_negative(self):
        result = self.calculate(cash_spent_ticks=10_200)

        self.assertEqual(result["average_fill_price_ticks"], 102.0)
        self.assertEqual(result["improvement_ticks"], -1.0)
        self.assertEqual(result["score"], -1.0)
        self.assertTrue(result["completed"])

    def test_invalid_episode_scores_zero(self):
        result = self.calculate(invalid_reason="invalid fixture action")

        self.assertFalse(result["valid"])
        self.assertFalse(result["completed"])
        self.assertEqual(result["score"], 0)
        self.assertEqual(result["invalid_reason"], "invalid fixture action")

    def test_builtin_reference_matches_golden_result(self):
        result, _ = evaluate_challenge(
            self.challenge,
            simple_reference_limit_then_market_cleanup,
            "simple_reference_limit_then_market_cleanup",
        )

        self.assert_golden(result, "builtin_simple_reference.golden.json")

    def test_invalid_action_matches_golden_result(self):
        def invalid_action_fixture(book, portfolio):
            del book, portfolio
            return [{"type": "bad_action"}]

        result, _ = evaluate_challenge(
            self.challenge,
            invalid_action_fixture,
            "invalid_action_fixture",
        )

        self.assert_golden(result, "invalid_action.golden.json")

    def test_incomplete_strategy_matches_golden_result(self):
        def incomplete_fixture(book, portfolio):
            del book, portfolio
            return []

        result, _ = evaluate_challenge(
            self.challenge,
            incomplete_fixture,
            "incomplete_fixture",
        )

        self.assert_golden(result, "incomplete.golden.json")

    def test_result_json_is_byte_stable(self):
        first, _ = evaluate_challenge(
            self.challenge,
            simple_reference_limit_then_market_cleanup,
            "simple_reference_limit_then_market_cleanup",
        )
        second, _ = evaluate_challenge(
            self.challenge,
            simple_reference_limit_then_market_cleanup,
            "simple_reference_limit_then_market_cleanup",
        )

        with tempfile.TemporaryDirectory() as temp_dir:
            first_path = Path(temp_dir) / "first.json"
            second_path = Path(temp_dir) / "second.json"
            write_result(first_path, first)
            write_result(second_path, second)
            self.assertEqual(first_path.read_bytes(), second_path.read_bytes())


if __name__ == "__main__":
    unittest.main()
