import copy
import contextlib
import io
import json
import tempfile
import unittest
from pathlib import Path

from arena.tools.evaluate_execution_v1 import (
    evaluate_challenge,
    evaluate_episode,
    load_challenge,
    main,
    simple_reference_limit_then_market_cleanup,
    write_result,
    write_replay,
)
from arena.tools.replay_schema import (
    REPLAY_SCHEMA_VERSION,
    ReplaySchemaError,
    validate_replay_records,
)


ROOT = Path(__file__).resolve().parents[2]
CHALLENGE_PATH = ROOT / "arena/challenges/beat_market_order.v1.json"
BUILTIN_REPLAY_FIXTURE = (
    ROOT
    / "arena/tests/fixtures/replays"
    / "builtin_simple_reference.seed7.replay.jsonl"
)
VIEWER_SAMPLE = (
    ROOT
    / "ui/replay-visualiser/public"
    / "arena-simple-reference.replay.jsonl"
)
VIEWER_RESULT_SAMPLE = (
    ROOT
    / "ui/replay-visualiser/public"
    / "arena-simple-reference.result.json"
)


class ReplaySchemaTest(unittest.TestCase):
    def setUp(self):
        self.challenge = load_challenge(CHALLENGE_PATH)

    def test_replay_records_validate_and_are_ordered(self):
        result, replay = evaluate_challenge(
            self.challenge,
            simple_reference_limit_then_market_cleanup,
            "simple_reference_limit_then_market_cleanup",
        )

        validate_replay_records(replay)
        self.assertEqual(
            [record["record_index"] for record in replay],
            list(range(len(replay))),
        )
        for seed in result["evaluation"]["seeds"]:
            episode = [record for record in replay if record["seed"] == seed]
            self.assertEqual(episode[0]["type"], "episode_start")
            self.assertEqual(episode[-1]["type"], "episode_result")
            self.assertEqual(
                [record["episode_sequence"] for record in episode],
                list(range(len(episode))),
            )

    def test_result_replay_metadata_matches_artifact(self):
        result, replay = evaluate_challenge(
            self.challenge,
            simple_reference_limit_then_market_cleanup,
            "simple_reference_limit_then_market_cleanup",
        )

        self.assertEqual(
            result["replay"]["schema_version"], REPLAY_SCHEMA_VERSION
        )
        self.assertEqual(result["replay"]["format"], "jsonl")
        self.assertEqual(result["replay"]["record_count"], len(replay))
        self.assertEqual(
            result["replay"]["seeds"], result["evaluation"]["seeds"]
        )
        self.assertEqual(result["replay"]["artifact_name"], "replay.jsonl")

    def test_cli_records_portable_replay_artifact_name(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            result_path = Path(temp_dir) / "result.json"
            replay_path = Path(temp_dir) / "portable.replay.jsonl"
            with contextlib.redirect_stdout(io.StringIO()):
                exit_code = main(
                    [
                        "--challenge",
                        str(CHALLENGE_PATH),
                        "--strategy",
                        "simple_reference",
                        "--seed-set",
                        "public",
                        "--results-out",
                        str(result_path),
                        "--replay-out",
                        str(replay_path),
                    ]
                )

            self.assertEqual(exit_code, 0)
            result = json.loads(result_path.read_text(encoding="utf-8"))
            self.assertEqual(
                result["replay"]["artifact_name"],
                "portable.replay.jsonl",
            )
            self.assertNotIn("path", result["replay"])

    def test_builtin_seed_replay_matches_golden_fixture(self):
        _, replay = evaluate_episode(
            self.challenge,
            7,
            simple_reference_limit_then_market_cleanup,
            "simple_reference_limit_then_market_cleanup",
            baseline_vwap_ticks=10001.8,
        )

        with tempfile.TemporaryDirectory() as temp_dir:
            actual_path = Path(temp_dir) / "actual.replay.jsonl"
            write_replay(actual_path, replay)
            self.assertEqual(
                actual_path.read_bytes(), BUILTIN_REPLAY_FIXTURE.read_bytes()
            )

    def test_viewer_sample_matches_validated_builtin_fixture(self):
        self.assertEqual(
            VIEWER_SAMPLE.read_bytes(), BUILTIN_REPLAY_FIXTURE.read_bytes()
        )

    def test_viewer_result_sample_matches_evaluator(self):
        challenge = copy.deepcopy(self.challenge)
        challenge["episodes"]["public"] = [7]
        result, _ = evaluate_challenge(
            challenge,
            simple_reference_limit_then_market_cleanup,
            "simple_reference_limit_then_market_cleanup",
            seed_set="public",
        )
        result["replay"]["artifact_name"] = VIEWER_SAMPLE.name

        with tempfile.TemporaryDirectory() as temp_dir:
            actual_path = Path(temp_dir) / "actual.result.json"
            write_result(actual_path, result)
            self.assertEqual(
                actual_path.read_bytes(), VIEWER_RESULT_SAMPLE.read_bytes()
            )

    def test_replay_serialization_is_byte_stable(self):
        _, first = evaluate_challenge(
            copy.deepcopy(self.challenge),
            simple_reference_limit_then_market_cleanup,
            "simple_reference_limit_then_market_cleanup",
        )
        _, second = evaluate_challenge(
            copy.deepcopy(self.challenge),
            simple_reference_limit_then_market_cleanup,
            "simple_reference_limit_then_market_cleanup",
        )

        with tempfile.TemporaryDirectory() as temp_dir:
            first_path = Path(temp_dir) / "first.replay.jsonl"
            second_path = Path(temp_dir) / "second.replay.jsonl"
            write_replay(first_path, first)
            write_replay(second_path, second)
            self.assertEqual(first_path.read_bytes(), second_path.read_bytes())

    def test_unsupported_schema_version_is_rejected(self):
        _, replay = evaluate_episode(
            self.challenge,
            7,
            simple_reference_limit_then_market_cleanup,
            "simple_reference_limit_then_market_cleanup",
            baseline_vwap_ticks=10001.8,
        )
        replay[0]["replay_schema_version"] = "999"

        with self.assertRaisesRegex(
            ReplaySchemaError, "unsupported replay schema version"
        ):
            validate_replay_records(replay)

    def test_missing_required_book_state_is_rejected(self):
        _, replay = evaluate_episode(
            self.challenge,
            7,
            simple_reference_limit_then_market_cleanup,
            "simple_reference_limit_then_market_cleanup",
            baseline_vwap_ticks=10001.8,
        )
        book_record = next(
            record for record in replay if record["type"] == "book_update"
        )
        del book_record["book"]

        with self.assertRaisesRegex(ReplaySchemaError, "book must be an object"):
            validate_replay_records(replay)


if __name__ == "__main__":
    unittest.main()
