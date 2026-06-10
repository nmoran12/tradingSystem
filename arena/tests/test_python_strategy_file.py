import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

from arena.tools.cpp_strategy_process import (
    PythonStrategyProcess,
    build_book_update_message,
)
from arena.tools.evaluate_execution_v1 import (
    evaluate_challenge,
    load_challenge,
)


ROOT = Path(__file__).resolve().parents[2]
CHALLENGE_PATH = ROOT / "arena/challenges/beat_market_order.v1.json"
BOOK_FIXTURE = ROOT / "arena/protocol/fixtures/book_update.sample.json"
ACTIONS_FIXTURE = ROOT / "arena/protocol/fixtures/actions.sample.json"
PYTHON_STRATEGY = ROOT / "arena/python/examples/simple_reference_strategy.py"


class PythonStrategyFileTest(unittest.TestCase):
    def setUp(self):
        self.challenge = load_challenge(CHALLENGE_PATH)

    def evaluate_process(self, command, timeout_ms=1000):
        strategy = PythonStrategyProcess(command, timeout_ms)
        return evaluate_challenge(
            self.challenge,
            strategy,
            strategy.strategy_name,
        )

    def write_script(self, body: str) -> Path:
        temp_dir = tempfile.TemporaryDirectory()
        self.addCleanup(temp_dir.cleanup)
        script_path = Path(temp_dir.name) / "strategy.py"
        script_path.write_text(body, encoding="utf-8")
        return script_path

    def test_python_strategy_file_completes_example_challenge(self):
        result, replay = self.evaluate_process(PYTHON_STRATEGY)

        self.assertTrue(result["evaluation"]["all_completed"])
        self.assertEqual(result["strategy"]["kind"], "python_file")
        self.assertEqual(
            result["strategy"]["mode"],
            "python_file",
        )
        self.assertEqual(
            result["strategy"]["identifier"],
            PYTHON_STRATEGY.relative_to(ROOT).as_posix(),
        )
        self.assertEqual(len(result["episodes"]), 3)
        self.assertTrue(replay)

    def test_python_strategy_file_is_deterministic(self):
        first_result, first_replay = self.evaluate_process(PYTHON_STRATEGY)
        second_result, second_replay = self.evaluate_process(PYTHON_STRATEGY)

        self.assertEqual(first_result, second_result)
        self.assertEqual(first_replay, second_replay)

    def test_python_strategy_file_matches_builtin_score(self):
        from arena.tools.evaluate_execution_v1 import BUILT_IN_STRATEGIES

        builtin_name, builtin_strategy = BUILT_IN_STRATEGIES["simple_reference"]
        builtin_result, _ = evaluate_challenge(
            self.challenge,
            builtin_strategy,
            builtin_name,
        )
        python_result, _ = self.evaluate_process(PYTHON_STRATEGY)

        self.assertEqual(
            python_result["scoring"]["aggregate_score"],
            builtin_result["scoring"]["aggregate_score"],
        )
        self.assertEqual(
            [episode["score"] for episode in python_result["episodes"]],
            [episode["score"] for episode in builtin_result["episodes"]],
        )

    def test_python_strategy_file_emits_expected_response(self):
        expected_book_update = json.loads(BOOK_FIXTURE.read_text(encoding="utf-8"))
        expected_actions = json.loads(ACTIONS_FIXTURE.read_text(encoding="utf-8"))

        payload = build_book_update_message(
            123,
            {
                "event_index": 0,
                "events_remaining": 99,
                "timestamp_ms": 0,
                "symbol": "ARENA",
                "bids": [{"price_ticks": 99, "quantity": 100}],
                "asks": [{"price_ticks": 101, "quantity": 100}],
            },
            {
                "target_quantity": 500,
                "filled_quantity": 0,
                "remaining_quantity": 500,
                "total_cost_tick_units": 0,
                "average_fill_price_ticks": None,
                "open_orders": [],
            },
        )
        self.assertEqual(payload, expected_book_update)

        process = subprocess.run(
            [sys.executable, str(PYTHON_STRATEGY)],
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

    def test_invalid_json_invalidates_episode(self):
        script = self.write_script(
            "import sys\n"
            "for line in sys.stdin:\n"
            "    if 'book_update' in line:\n"
            "        print('not-json', flush=True)\n"
        )

        result, _ = self.evaluate_process(script)

        self.assertEqual(result["episodes"][0]["status"], "invalid")
        self.assertEqual(result["episodes"][0]["score"], 0)
        self.assertIn("not valid JSON", result["episodes"][0]["error_reason"])

    def test_missing_actions_type_invalidates_episode(self):
        script = self.write_script(
            "import sys\n"
            "for line in sys.stdin:\n"
            "    if 'book_update' in line:\n"
            "        print('{\"actions\":[]}', flush=True)\n"
        )

        result, _ = self.evaluate_process(script)

        self.assertEqual(result["episodes"][0]["status"], "invalid")
        self.assertEqual(result["episodes"][0]["score"], 0)
        self.assertIn(
            'must contain "type": "actions"',
            result["episodes"][0]["error_reason"],
        )

    def test_non_list_actions_invalidates_episode(self):
        script = self.write_script(
            "import sys\n"
            "for line in sys.stdin:\n"
            "    if 'book_update' in line:\n"
            "        print('{\"type\":\"actions\",\"actions\":42}', flush=True)\n"
        )

        result, _ = self.evaluate_process(script)

        self.assertEqual(result["episodes"][0]["status"], "invalid")
        self.assertEqual(result["episodes"][0]["score"], 0)
        self.assertIn('\"actions\" must be a list', result["episodes"][0]["error_reason"])

    def test_early_process_exit_invalidates_episode(self):
        script = self.write_script(
            "import sys\n"
            "sys.exit(0)\n"
        )

        result, _ = self.evaluate_process(script)

        self.assertEqual(result["episodes"][0]["status"], "invalid")
        self.assertEqual(result["episodes"][0]["score"], 0)
        self.assertIn(
            "exited before responding",
            result["episodes"][0]["error_reason"],
        )

    def test_timeout_invalidates_episode(self):
        script = self.write_script(
            "import sys\n"
            "import time\n"
            "for line in sys.stdin:\n"
            "    if 'book_update' in line:\n"
            "        time.sleep(1.5)\n"
            "        print('{\"type\":\"actions\",\"actions\":[]}', flush=True)\n"
        )

        result, _ = self.evaluate_process(script, timeout_ms=100)

        self.assertEqual(result["episodes"][0]["status"], "invalid")
        self.assertEqual(result["episodes"][0]["score"], 0)
        self.assertIn("timed out", result["episodes"][0]["error_reason"])


if __name__ == "__main__":
    unittest.main()
