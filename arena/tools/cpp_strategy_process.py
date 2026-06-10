"""Trusted-local JSONL adapter for execution_v1 C++ strategy processes."""

from __future__ import annotations

import copy
import json
import selectors
import subprocess
import sys
from pathlib import Path
from typing import Any, Sequence


PROTOCOL_VERSION = "1.0"


class StrategyProcessError(RuntimeError):
    """Raised when a local strategy process violates the JSONL contract."""


def _portable_strategy_label(command: str | Path) -> str:
    path = Path(command)
    if not path.is_absolute():
        return path.as_posix()

    cwd = Path.cwd().resolve()
    try:
        return path.resolve().relative_to(cwd).as_posix()
    except (OSError, ValueError):
        return path.name


def build_book_update_message(
    episode_seed: int,
    book: dict[str, Any],
    portfolio: dict[str, Any],
) -> dict[str, Any]:
    return {
        "type": "book_update",
        "protocol_version": PROTOCOL_VERSION,
        "episode_seed": episode_seed,
        "event_index": book["event_index"],
        "events_remaining": book["events_remaining"],
        "timestamp_ms": book["timestamp_ms"],
        "symbol": book["symbol"],
        "book": {
            "bids": copy.deepcopy(book["bids"]),
            "asks": copy.deepcopy(book["asks"]),
        },
        "portfolio": {
            "target_quantity": portfolio["target_quantity"],
            "filled_quantity": portfolio["filled_quantity"],
            "remaining_quantity": portfolio["remaining_quantity"],
            "cash_spent_ticks": portfolio["total_cost_tick_units"],
            "average_fill_price_ticks": portfolio["average_fill_price_ticks"],
            "open_orders": [
                {
                    "order_id": order["order_id"],
                    "side": "buy",
                    "type": order["type"],
                    "price_ticks": order["price_ticks"],
                    "quantity": order["remaining_quantity"],
                }
                for order in portfolio["open_orders"]
            ],
        },
    }


class JsonlStrategyProcess:
    """Trusted local JSONL strategy process used for a complete evaluation."""

    strategy_kind = "external_process"

    def __init__(
        self,
        command: str | Path | Sequence[str],
        timeout_ms: int,
        *,
        strategy_name: str | None = None,
        strategy_kind: str | None = None,
    ):
        if isinstance(command, (str, Path)):
            self.command = [str(command)]
        else:
            self.command = [str(part) for part in command]
        if not self.command:
            raise ValueError("strategy process command must not be empty")
        if timeout_ms <= 0:
            raise ValueError("strategy process timeout must be positive")

        self.timeout_seconds = timeout_ms / 1000
        self.current_seed: int | None = None
        self._closed = False
        self._strategy_name = strategy_name or Path(self.command[0]).name
        self.strategy_kind = strategy_kind or self.strategy_kind
        try:
            self.process = subprocess.Popen(
                self.command,
                stdin=subprocess.PIPE,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
                bufsize=1,
            )
        except OSError as error:
            raise StrategyProcessError(
                f"could not start strategy process {self.command[0]!r}: {error}"
            ) from error

        if self.process.stdin is None or self.process.stdout is None:
            self._terminate()
            raise StrategyProcessError("strategy process pipes were not created")

    @property
    def strategy_name(self) -> str:
        return self._strategy_name

    def begin_episode(self, seed: int) -> None:
        self._require_running()
        self.current_seed = seed

    def __call__(
        self,
        book: dict[str, Any],
        portfolio: dict[str, Any],
    ) -> list[dict[str, Any]]:
        if self.current_seed is None:
            raise StrategyProcessError("strategy process episode was not started")

        self._send(
            build_book_update_message(self.current_seed, book, portfolio)
        )
        response = self._read_response()
        if response.get("type") != "actions":
            raise StrategyProcessError(
                'strategy response must contain "type": "actions"'
            )
        actions = response.get("actions")
        if not isinstance(actions, list):
            raise StrategyProcessError(
                'strategy response "actions" must be a list'
            )
        return actions

    def end_episode(self, seed: int) -> None:
        if self.current_seed is None:
            return
        self._require_running()
        self._send(
            {
                "type": "episode_end",
                "protocol_version": PROTOCOL_VERSION,
                "episode_seed": seed,
            }
        )
        self.current_seed = None

    def close(self) -> None:
        if self._closed:
            return
        self._closed = True
        if self.process.poll() is None:
            try:
                self._send(
                    {
                        "type": "evaluation_end",
                        "protocol_version": PROTOCOL_VERSION,
                    }
                )
                assert self.process.stdin is not None
                self.process.stdin.close()
                self.process.wait(timeout=self.timeout_seconds)
            except (BrokenPipeError, OSError, subprocess.TimeoutExpired):
                self._terminate()
                return
        self._close_pipes()

    def _send(self, message: dict[str, Any]) -> None:
        self._require_running()
        assert self.process.stdin is not None
        try:
            self.process.stdin.write(
                json.dumps(message, separators=(",", ":"))
            )
            self.process.stdin.write("\n")
            self.process.stdin.flush()
        except (BrokenPipeError, OSError) as error:
            raise StrategyProcessError(
                f"strategy process input failed: {self._exit_detail()}"
            ) from error

    def _read_response(self) -> dict[str, Any]:
        assert self.process.stdout is not None
        selector = selectors.DefaultSelector()
        try:
            selector.register(self.process.stdout, selectors.EVENT_READ)
            ready = selector.select(self.timeout_seconds)
        finally:
            selector.close()

        if not ready:
            self._terminate()
            raise StrategyProcessError(
                f"strategy response timed out after "
                f"{int(self.timeout_seconds * 1000)} ms"
            )

        line = self.process.stdout.readline()
        if line == "":
            raise StrategyProcessError(
                f"strategy process exited before responding: {self._exit_detail()}"
            )

        try:
            response = json.loads(line)
        except json.JSONDecodeError as error:
            raise StrategyProcessError(
                f"strategy response is not valid JSON: {error.msg}"
            ) from error
        if not isinstance(response, dict):
            raise StrategyProcessError("strategy response must be a JSON object")
        return response

    def _require_running(self) -> None:
        return_code = self.process.poll()
        if return_code is not None:
            raise StrategyProcessError(
                f"strategy process is not running: {self._exit_detail()}"
            )

    def _exit_detail(self) -> str:
        return_code = self.process.poll()
        detail = (
            f"exit code {return_code}"
            if return_code is not None
            else "process still running"
        )
        if return_code is not None and self.process.stderr is not None:
            stderr = self.process.stderr.read().strip()
            if stderr:
                detail += f"; stderr: {stderr[:500]}"
        return detail

    def _terminate(self) -> None:
        if self.process.poll() is None:
            self.process.kill()
            try:
                self.process.wait(timeout=1)
            except subprocess.TimeoutExpired:
                pass
        self._close_pipes()

    def _close_pipes(self) -> None:
        for pipe in (
            self.process.stdin,
            self.process.stdout,
            self.process.stderr,
        ):
            if pipe is not None and not pipe.closed:
                pipe.close()

    def __enter__(self) -> "CppStrategyProcess":
        return self

    def __exit__(self, exc_type, exc_value, traceback) -> None:
        del exc_type, exc_value, traceback
        self.close()


class CppStrategyProcess(JsonlStrategyProcess):
    """Trusted local C++ child process used for a complete evaluation."""

    strategy_kind = "external_cpp_process"

    def __init__(
        self,
        command: str | Path | Sequence[str],
        timeout_ms: int,
    ):
        if isinstance(command, (str, Path)):
            command_list = [str(command)]
        else:
            command_list = [str(part) for part in command]
        super().__init__(
            command_list,
            timeout_ms,
            strategy_name=Path(command_list[0]).name,
            strategy_kind=self.strategy_kind,
        )


class PythonStrategyProcess(JsonlStrategyProcess):
    """Trusted local Python file used for a complete evaluation."""

    strategy_kind = "python_file"

    def __init__(
        self,
        script_path: str | Path,
        timeout_ms: int,
    ):
        script = Path(script_path)
        super().__init__(
            [sys.executable, str(script)],
            timeout_ms,
            strategy_name=_portable_strategy_label(script),
            strategy_kind=self.strategy_kind,
        )
