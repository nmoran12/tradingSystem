import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

from arena.tools.cpp_strategy_process import (
    CppStrategyProcess,
    build_book_update_message,
)
from arena.tools.evaluate_execution_v1 import evaluate_challenge, load_challenge


ROOT = Path(__file__).resolve().parents[2]
CHALLENGE_PATH = ROOT / "arena/challenges/beat_market_order.v1.json"
CPP_SOURCE_DIR = ROOT / "arena/cpp"
BOOK_FIXTURE = (
    ROOT / "arena/protocol/fixtures/book_update.sample.json"
)
ACTIONS_FIXTURE = ROOT / "arena/protocol/fixtures/actions.sample.json"


class CppStrategyProcessTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp_dir = tempfile.TemporaryDirectory()
        cls.build_dir = Path(cls.temp_dir.name) / "build"
        subprocess.run(
            ["cmake", "-S", str(CPP_SOURCE_DIR), "-B", str(cls.build_dir)],
            check=True,
            capture_output=True,
            text=True,
        )
        subprocess.run(
            ["cmake", "--build", str(cls.build_dir)],
            check=True,
            capture_output=True,
            text=True,
        )
        cls.strategy_binary = (
            cls.build_dir / "arena_simple_reference_strategy"
        )

    @classmethod
    def tearDownClass(cls):
        cls.temp_dir.cleanup()

    def setUp(self):
        self.challenge = load_challenge(CHALLENGE_PATH)

    def evaluate_process(self, command, timeout_ms=1000):
        strategy = CppStrategyProcess(command, timeout_ms)
        return evaluate_challenge(
            self.challenge,
            strategy,
            strategy.strategy_name,
        )

    def test_cpp_strategy_process_completes_example_challenge(self):
        result, replay = self.evaluate_process(self.strategy_binary)

        self.assertTrue(result["evaluation"]["all_completed"])
        self.assertEqual(
            result["strategy"]["kind"], "external_cpp_process"
        )
        self.assertEqual(len(result["episodes"]), 3)
        self.assertTrue(replay)

    def test_cpp_strategy_process_is_deterministic(self):
        first_result, first_replay = self.evaluate_process(
            self.strategy_binary
        )
        second_result, second_replay = self.evaluate_process(
            self.strategy_binary
        )

        self.assertEqual(first_result, second_result)
        self.assertEqual(first_replay, second_replay)

    def test_normalized_payload_matches_fixture_and_cpp_response(self):
        expected_book_update = json.loads(BOOK_FIXTURE.read_text())
        expected_actions = json.loads(ACTIONS_FIXTURE.read_text())
        book = {
            "event_index": 0,
            "events_remaining": 99,
            "timestamp_ms": 0,
            "symbol": "ARENA",
            "bids": [{"price_ticks": 99, "quantity": 100}],
            "asks": [{"price_ticks": 101, "quantity": 100}],
        }
        portfolio = {
            "target_quantity": 500,
            "filled_quantity": 0,
            "remaining_quantity": 500,
            "total_cost_tick_units": 0,
            "average_fill_price_ticks": None,
            "open_orders": [],
        }

        payload = build_book_update_message(123, book, portfolio)
        self.assertEqual(payload, expected_book_update)

        process = subprocess.run(
            [str(self.strategy_binary)],
            input=(
                json.dumps(payload, separators=(",", ":"))
                + "\n"
                + '{"type":"evaluation_end","protocol_version":"1.0"}\n'
            ),
            check=True,
            capture_output=True,
            text=True,
        )
        response = json.loads(process.stdout.splitlines()[0])
        self.assertEqual(response, expected_actions)

        payload_with_nested_type = build_book_update_message(
            123,
            {
                **book,
                "event_index": 12,
                "events_remaining": 50,
                "timestamp_ms": 1200,
            },
            {
                **portfolio,
                "filled_quantity": 120,
                "remaining_quantity": 380,
                "total_cost_tick_units": 12120,
                "average_fill_price_ticks": 101,
                "open_orders": [
                    {
                        "order_id": 1,
                        "type": "limit_order",
                        "price_ticks": 99,
                        "remaining_quantity": 100,
                    }
                ],
            },
        )
        process = subprocess.run(
            [str(self.strategy_binary)],
            input=(
                json.dumps(payload_with_nested_type, sort_keys=True)
                + "\n"
                + '{"type":"evaluation_end","protocol_version":"1.0"}\n'
            ),
            check=True,
            capture_output=True,
            text=True,
        )
        response = json.loads(process.stdout.splitlines()[0])
        self.assertEqual(response, {"type": "actions", "actions": []})

    def test_invalid_json_invalidates_episode(self):
        command = [
            sys.executable,
            "-c",
            (
                "import sys\n"
                "for line in sys.stdin:\n"
                "    if 'book_update' in line:\n"
                "        print('not-json', flush=True)\n"
            ),
        ]

        result, _ = self.evaluate_process(command)

        self.assertEqual(result["episodes"][0]["status"], "invalid")
        self.assertEqual(result["episodes"][0]["score"], 0)
        self.assertIn(
            "not valid JSON", result["episodes"][0]["error_reason"]
        )

    def test_missing_actions_type_invalidates_episode(self):
        command = [
            sys.executable,
            "-c",
            (
                "import sys\n"
                "for line in sys.stdin:\n"
                "    if 'book_update' in line:\n"
                "        print('{\"actions\":[]}', flush=True)\n"
            ),
        ]

        result, _ = self.evaluate_process(command)

        self.assertEqual(result["episodes"][0]["status"], "invalid")
        self.assertIn(
            '"type": "actions"', result["episodes"][0]["error_reason"]
        )

    def test_non_list_actions_invalidates_episode(self):
        command = [
            sys.executable,
            "-c",
            (
                "import sys\n"
                "for line in sys.stdin:\n"
                "    if 'book_update' in line:\n"
                "        print('{\"type\":\"actions\",\"actions\":{}}', "
                "flush=True)\n"
            ),
        ]

        result, _ = self.evaluate_process(command)

        self.assertEqual(result["episodes"][0]["status"], "invalid")
        self.assertIn(
            '"actions" must be a list',
            result["episodes"][0]["error_reason"],
        )

    def test_early_process_exit_invalidates_episode(self):
        command = [
            sys.executable,
            "-c",
            "import sys; sys.stdin.readline(); sys.exit(7)",
        ]

        result, _ = self.evaluate_process(command)

        self.assertEqual(result["episodes"][0]["status"], "invalid")
        self.assertIn(
            "exited before responding",
            result["episodes"][0]["error_reason"],
        )

    def test_timeout_invalidates_episode(self):
        command = [
            sys.executable,
            "-c",
            "import sys, time; sys.stdin.readline(); time.sleep(2)",
        ]

        result, _ = self.evaluate_process(command, timeout_ms=50)

        self.assertEqual(result["episodes"][0]["status"], "invalid")
        self.assertIn("timed out", result["episodes"][0]["error_reason"])


if __name__ == "__main__":
    unittest.main()
