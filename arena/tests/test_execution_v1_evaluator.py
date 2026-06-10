import copy
import unittest
from pathlib import Path

from arena.tools.evaluate_execution_v1 import (
    SplitMix64,
    evaluate_challenge,
    evaluate_episode,
    load_challenge,
    simple_reference_limit_then_market_cleanup,
)


ROOT = Path(__file__).resolve().parents[2]
CHALLENGE_PATH = ROOT / "arena/challenges/beat_market_order.v1.json"


class ExecutionV1EvaluatorTest(unittest.TestCase):
    def setUp(self):
        self.challenge = load_challenge(CHALLENGE_PATH)

    def test_challenge_config_loads(self):
        self.assertEqual(self.challenge["challenge_type"], "execution_v1")
        self.assertEqual(self.challenge["title"], "Beat the Market Order")

    def test_splitmix64_known_vector(self):
        generator = SplitMix64(0)

        self.assertEqual(
            [generator.next_u64() for _ in range(3)],
            [
                0xE220A8397B1DCDAF,
                0x6E789E6AA1B965F4,
                0x06C45D188009454F,
            ],
        )

    def test_same_seed_gives_same_result(self):
        first_result, first_replay = evaluate_episode(
            self.challenge,
            901,
            simple_reference_limit_then_market_cleanup,
            "simple_reference_limit_then_market_cleanup",
            baseline_vwap_ticks=10001.8,
        )
        second_result, second_replay = evaluate_episode(
            self.challenge,
            901,
            simple_reference_limit_then_market_cleanup,
            "simple_reference_limit_then_market_cleanup",
            baseline_vwap_ticks=10001.8,
        )

        self.assertEqual(first_result, second_result)
        self.assertEqual(first_replay, second_replay)

    def test_invalid_action_invalidates_episode(self):
        def invalid_strategy(book, portfolio):
            del book, portfolio
            return [{"type": "sell_everything"}]

        result, _ = evaluate_episode(
            self.challenge,
            901,
            invalid_strategy,
            "invalid_strategy",
            baseline_vwap_ticks=10001.8,
        )

        self.assertEqual(result["status"], "invalid")
        self.assertFalse(result["completed"])
        self.assertEqual(result["score"], 0)
        self.assertIn("unsupported action type", result["error_reason"])

    def test_full_completion_is_required(self):
        def no_action_strategy(book, portfolio):
            del book, portfolio
            return []

        result, _ = evaluate_episode(
            self.challenge,
            901,
            no_action_strategy,
            "no_action_strategy",
            baseline_vwap_ticks=10001.8,
        )

        self.assertEqual(result["status"], "incomplete")
        self.assertFalse(result["completed"])
        self.assertEqual(result["score"], 0)
        self.assertEqual(result["remaining_quantity"], 500)

    def test_aggregate_score_includes_all_local_evaluation_seeds(self):
        result, _ = evaluate_challenge(
            self.challenge,
            simple_reference_limit_then_market_cleanup,
            "simple_reference_limit_then_market_cleanup",
        )

        expected_seeds = self.challenge["episodes"]["local_evaluation"]
        episode_scores = [episode["score"] for episode in result["episodes"]]
        self.assertEqual(result["evaluation"]["seeds"], expected_seeds)
        self.assertEqual(len(result["episodes"]), len(expected_seeds))
        self.assertEqual(
            result["scoring"]["aggregate_score"],
            round(sum(episode_scores) / len(expected_seeds), 6),
        )

    def test_baseline_and_reference_are_deterministic(self):
        from arena.tools.evaluate_execution_v1 import BUILT_IN_STRATEGIES

        for strategy_key in ("baseline", "simple_reference"):
            strategy_name, strategy = BUILT_IN_STRATEGIES[strategy_key]
            first, first_replay = evaluate_challenge(
                copy.deepcopy(self.challenge), strategy, strategy_name
            )
            second, second_replay = evaluate_challenge(
                copy.deepcopy(self.challenge), strategy, strategy_name
            )
            self.assertEqual(first, second)
            self.assertEqual(first_replay, second_replay)


if __name__ == "__main__":
    unittest.main()
