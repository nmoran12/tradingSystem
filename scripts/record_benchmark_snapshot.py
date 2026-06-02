#!/usr/bin/env python3
"""Record a local Release benchmark snapshot.

The script intentionally wraps the existing benchmark shell scripts instead of
adding another benchmark path. It captures raw output, parses the metrics that
the current benchmark binaries print, appends a JSONL history entry, and
regenerates a static local dashboard.
"""

from __future__ import annotations

import datetime as dt
import html
import json
import os
import platform
import re
import statistics
import subprocess
import sys
from pathlib import Path
from typing import Iterable


ROOT = Path(__file__).resolve().parents[1]
HISTORY_DIR = ROOT / "benchmark-history"
RAW_DIR = HISTORY_DIR / "raw"
HISTORY_FILE = HISTORY_DIR / "benchmark_history.jsonl"
DASHBOARD_FILE = HISTORY_DIR / "dashboard.html"

METRIC_KEYS = (
    "binaryWriteCommandsPerSec",
    "binaryWriteNsPerCommand",
    "bufferedReadDecodeCommandsPerSec",
    "bufferedReadDecodeNsPerCommand",
    "bufferedEngineApplyCommandsPerSec",
    "bufferedEngineApplyNsPerCommand",
    "streamingReadDecodeApplyCommandsPerSec",
    "streamingReadDecodeApplyNsPerCommand",
    "matchingEngineCommandsPerSec",
    "matchingEngineAvgNs",
    "matchingEngineP50Ns",
    "matchingEngineP95Ns",
    "matchingEngineP99Ns",
)

PHASE_TO_KEYS = {
    "binary write": ("binaryWriteCommandsPerSec", "binaryWriteNsPerCommand"),
    "buffered read/decode": (
        "bufferedReadDecodeCommandsPerSec",
        "bufferedReadDecodeNsPerCommand",
    ),
    "buffered engine apply": (
        "bufferedEngineApplyCommandsPerSec",
        "bufferedEngineApplyNsPerCommand",
    ),
    "streaming read/decode/apply": (
        "streamingReadDecodeApplyCommandsPerSec",
        "streamingReadDecodeApplyNsPerCommand",
    ),
}


def run_text(command: list[str]) -> str:
    completed = subprocess.run(
        command,
        cwd=ROOT,
        check=False,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    return completed.stdout.strip()


def run_and_capture(command: list[str], output_path: Path) -> int:
    with output_path.open("a", encoding="utf-8") as output:
        output.write(f"$ {' '.join(command)}\n\n")
        output.flush()

        process = subprocess.Popen(
            command,
            cwd=ROOT,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
        )
        assert process.stdout is not None
        for line in process.stdout:
            print(line, end="")
            output.write(line)
        process.wait()
        output.write(f"\n[exit_code] {process.returncode}\n")
        return process.returncode


def compiler_info() -> str | None:
    for candidate in (os.environ.get("CXX"), "c++", "clang++", "g++"):
        if not candidate:
            continue
        try:
            text = run_text([candidate, "--version"])
        except OSError:
            continue
        first_line = text.splitlines()[0] if text else ""
        if first_line:
            return first_line
    return None


def median_int(values: Iterable[int]) -> int | None:
    values_list = list(values)
    if not values_list:
        return None
    return int(statistics.median(values_list))


def split_runs(raw_text: str) -> list[str]:
    parts = re.split(r"^== Run \d+ of \d+ ==\s*$", raw_text, flags=re.MULTILINE)
    runs = [part for part in parts[1:] if "Benchmark:" in part]
    return runs if runs else [raw_text]


def parse_run(run_text: str) -> dict[str, int]:
    metrics: dict[str, int] = {}

    for label, (throughput_key, ns_key) in PHASE_TO_KEYS.items():
        pattern = (
            rf"^\s*{re.escape(label)}:\s*[-+0-9.eE]+s "
            rf"\(([0-9]+) commands/sec, ([0-9]+) ns/command\)"
        )
        match = re.search(pattern, run_text, flags=re.MULTILINE)
        if match:
            metrics[throughput_key] = int(match.group(1))
            metrics[ns_key] = int(match.group(2))

    matching_block_match = re.search(
        r"Benchmark: MatchingEngine synthetic workload(?P<body>.*?)(?:\n== |\Z)",
        run_text,
        flags=re.DOTALL,
    )
    if matching_block_match:
        body = matching_block_match.group("body")
        throughput = re.search(r"^Throughput:\s*([0-9]+) commands/sec$", body, re.MULTILINE)
        if throughput:
            metrics["matchingEngineCommandsPerSec"] = int(throughput.group(1))
        latency_map = {
            "avg": "matchingEngineAvgNs",
            "p50": "matchingEngineP50Ns",
            "p95": "matchingEngineP95Ns",
            "p99": "matchingEngineP99Ns",
        }
        for label, key in latency_map.items():
            match = re.search(rf"^\s*{label}:\s*([0-9]+)$", body, re.MULTILINE)
            if match:
                metrics[key] = int(match.group(1))

    return metrics


def parse_metrics(raw_text: str) -> dict[str, int | None]:
    runs = [parse_run(run) for run in split_runs(raw_text)]
    return {
        key: median_int(run[key] for run in runs if key in run)
        for key in METRIC_KEYS
    }


def load_history() -> list[dict]:
    if not HISTORY_FILE.exists():
        return []
    entries: list[dict] = []
    for line in HISTORY_FILE.read_text(encoding="utf-8").splitlines():
        stripped = line.strip()
        if not stripped:
            continue
        try:
            entries.append(json.loads(stripped))
        except json.JSONDecodeError:
            continue
    return entries


def fmt_number(value: object) -> str:
    if isinstance(value, (int, float)):
        return f"{value:,}"
    return "n/a"


def write_dashboard(entries: list[dict]) -> None:
    json_payload = json.dumps(entries, indent=2, sort_keys=True).replace("</", "<\\/")
    generated_at = dt.datetime.now(dt.timezone.utc).isoformat(timespec="seconds")
    DASHBOARD_FILE.write_text(
        f"""<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Order Book Benchmark History</title>
  <style>
    :root {{
      color-scheme: dark;
      font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif;
      background: #020617;
      color: #e5e7eb;
    }}
    body {{ margin: 0; padding: 24px; }}
    .wrap {{ max-width: 1180px; margin: 0 auto; }}
    h1 {{ margin: 0 0 8px; font-size: 1.8rem; }}
    .muted {{ color: #94a3b8; }}
    .warning {{
      margin: 18px 0;
      padding: 12px 14px;
      border: 1px solid rgba(251, 191, 36, 0.35);
      background: rgba(251, 191, 36, 0.08);
      border-radius: 12px;
      color: #fde68a;
    }}
    .panel {{
      margin-top: 18px;
      padding: 16px;
      border: 1px solid rgba(51, 65, 85, 0.9);
      border-radius: 14px;
      background: rgba(15, 23, 42, 0.78);
    }}
    .legend {{ display: flex; flex-wrap: wrap; gap: 12px; margin-top: 12px; }}
    .legend span {{ display: inline-flex; align-items: center; gap: 6px; color: #cbd5e1; }}
    .dot {{ width: 10px; height: 10px; border-radius: 999px; display: inline-block; }}
    svg {{ width: 100%; height: 360px; display: block; }}
    table {{ width: 100%; border-collapse: collapse; margin-top: 12px; font-size: 0.9rem; }}
    th, td {{ padding: 8px 10px; border-bottom: 1px solid rgba(51, 65, 85, 0.7); text-align: right; }}
    th:first-child, td:first-child, th:nth-child(2), td:nth-child(2) {{ text-align: left; }}
    th {{ color: #cbd5e1; font-weight: 600; }}
    code {{ color: #bae6fd; }}
  </style>
</head>
<body>
  <div class="wrap">
    <h1>Performance Baseline Dashboard</h1>
    <div class="muted">Generated {html.escape(generated_at)}. Data is embedded from <code>benchmark_history.jsonl</code>.</div>
    <div class="warning">Local benchmark results are machine-dependent. Compare repeated runs on the same machine, build type, command count, and seed.</div>

    <section class="panel">
      <h2>Throughput Over Time</h2>
      <svg id="chart" viewBox="0 0 1000 360" role="img" aria-label="Benchmark throughput over time"></svg>
      <div class="legend">
        <span><i class="dot" style="background:#38bdf8"></i> Matching engine</span>
        <span><i class="dot" style="background:#22c55e"></i> Buffered engine apply</span>
        <span><i class="dot" style="background:#f59e0b"></i> Streaming read/decode/apply</span>
      </div>
    </section>

    <section class="panel">
      <h2>Runs</h2>
      <div id="empty" class="muted"></div>
      <table>
        <thead>
          <tr>
            <th>Date/time</th>
            <th>Commit</th>
            <th>Commands / seed</th>
            <th>ME cmd/s</th>
            <th>p50</th>
            <th>p95</th>
            <th>p99</th>
            <th>Buffered apply cmd/s</th>
          </tr>
        </thead>
        <tbody id="rows"></tbody>
      </table>
    </section>
  </div>

  <script id="benchmark-data" type="application/json">{json_payload}</script>
  <script>
    const data = JSON.parse(document.getElementById('benchmark-data').textContent);
    const rows = document.getElementById('rows');
    const empty = document.getElementById('empty');
    const formatNumber = (value) => Number.isFinite(value) ? Math.round(value).toLocaleString() : 'n/a';
    if (data.length === 0) {{
      empty.textContent = 'No benchmark snapshots recorded yet.';
    }}
    for (const entry of data) {{
      const metrics = entry.metrics || {{}};
      const tr = document.createElement('tr');
      tr.innerHTML = `
        <td>${{new Date(entry.timestamp).toLocaleString()}}</td>
        <td><code>${{entry.gitCommit || 'unknown'}}</code></td>
        <td>${{entry.commands?.toLocaleString?.() || entry.commands}} / ${{entry.seed}}</td>
        <td>${{formatNumber(metrics.matchingEngineCommandsPerSec)}}</td>
        <td>${{formatNumber(metrics.matchingEngineP50Ns)}}</td>
        <td>${{formatNumber(metrics.matchingEngineP95Ns)}}</td>
        <td>${{formatNumber(metrics.matchingEngineP99Ns)}}</td>
        <td>${{formatNumber(metrics.bufferedEngineApplyCommandsPerSec)}}</td>
      `;
      rows.appendChild(tr);
    }}

    const svg = document.getElementById('chart');
    const series = [
      ['matchingEngineCommandsPerSec', '#38bdf8'],
      ['bufferedEngineApplyCommandsPerSec', '#22c55e'],
      ['streamingReadDecodeApplyCommandsPerSec', '#f59e0b'],
    ];
    const width = 1000, height = 360, pad = 52;
    const values = [];
    for (const entry of data) {{
      for (const [key] of series) {{
        const value = entry.metrics?.[key];
        if (Number.isFinite(value)) values.push(value);
      }}
    }}
    const maxY = Math.max(1, ...values) * 1.08;
    const minY = 0;
    const xFor = (index) => data.length <= 1 ? width / 2 : pad + (index / (data.length - 1)) * (width - pad * 2);
    const yFor = (value) => height - pad - ((value - minY) / (maxY - minY)) * (height - pad * 2);
    const line = (x1, y1, x2, y2, color, opacity = 1) => {{
      const el = document.createElementNS('http://www.w3.org/2000/svg', 'line');
      el.setAttribute('x1', x1); el.setAttribute('y1', y1);
      el.setAttribute('x2', x2); el.setAttribute('y2', y2);
      el.setAttribute('stroke', color); el.setAttribute('stroke-opacity', opacity);
      el.setAttribute('stroke-width', '1');
      svg.appendChild(el);
    }};
    for (let i = 0; i <= 4; i += 1) {{
      const y = pad + i * ((height - pad * 2) / 4);
      line(pad, y, width - pad, y, '#334155', 0.7);
      const label = document.createElementNS('http://www.w3.org/2000/svg', 'text');
      const value = maxY - i * (maxY / 4);
      label.setAttribute('x', 6); label.setAttribute('y', y + 4);
      label.setAttribute('fill', '#94a3b8'); label.setAttribute('font-size', '12');
      label.textContent = formatNumber(value);
      svg.appendChild(label);
    }}
    for (const [key, color] of series) {{
      const points = data
        .map((entry, index) => [index, entry.metrics?.[key]])
        .filter(([, value]) => Number.isFinite(value));
      if (points.length === 0) continue;
      const path = document.createElementNS('http://www.w3.org/2000/svg', 'polyline');
      path.setAttribute('fill', 'none');
      path.setAttribute('stroke', color);
      path.setAttribute('stroke-width', '3');
      path.setAttribute('points', points.map(([index, value]) => `${{xFor(index).toFixed(1)}},${{yFor(value).toFixed(1)}}`).join(' '));
      svg.appendChild(path);
      for (const [index, value] of points) {{
        const dot = document.createElementNS('http://www.w3.org/2000/svg', 'circle');
        dot.setAttribute('cx', xFor(index)); dot.setAttribute('cy', yFor(value));
        dot.setAttribute('r', '4'); dot.setAttribute('fill', color);
        svg.appendChild(dot);
      }}
    }}
  </script>
</body>
</html>
""",
        encoding="utf-8",
    )


def main(argv: list[str]) -> int:
    run_count = int(argv[1]) if len(argv) > 1 else 5
    commands = int(argv[2]) if len(argv) > 2 else 100_000
    seed = int(argv[3]) if len(argv) > 3 else 42

    HISTORY_DIR.mkdir(exist_ok=True)
    RAW_DIR.mkdir(exist_ok=True)

    timestamp = dt.datetime.now(dt.timezone.utc).isoformat(timespec="seconds")
    safe_timestamp = timestamp.replace(":", "").replace("+", "Z")
    git_commit = run_text(["git", "rev-parse", "--short", "HEAD"]) or "unknown"
    branch = run_text(["git", "branch", "--show-current"]) or "unknown"
    raw_name = f"{safe_timestamp}_{git_commit}_benchmark.txt"
    raw_path = RAW_DIR / raw_name

    with raw_path.open("w", encoding="utf-8") as output:
        output.write("Benchmark snapshot raw output\n")
        output.write(f"Timestamp: {timestamp}\n")
        output.write(f"Git commit: {git_commit}\n")
        output.write(f"Branch: {branch}\n")
        output.write(f"Runs: {run_count}\n")
        output.write(f"Commands: {commands}\n")
        output.write(f"Seed: {seed}\n\n")

    return_code = run_and_capture(
        ["./scripts/benchmark_repeat.sh", str(run_count), str(commands), str(seed)],
        raw_path,
    )
    if return_code != 0:
        print(f"Benchmark run failed; raw output saved to {raw_path}", file=sys.stderr)
        return return_code

    raw_text = raw_path.read_text(encoding="utf-8")
    metrics = parse_metrics(raw_text)
    entry = {
        "timestamp": timestamp,
        "gitCommit": git_commit,
        "branch": branch,
        "machine": f"{platform.platform()} ({platform.machine()})",
        "compiler": compiler_info(),
        "buildType": "Release",
        "commands": commands,
        "seed": seed,
        "runCount": run_count,
        "verifyPassed": True,
        "metrics": metrics,
        "rawOutput": str(raw_path.relative_to(ROOT)),
        "notes": "local machine-dependent result; not a production claim",
    }

    with HISTORY_FILE.open("a", encoding="utf-8") as history:
        history.write(json.dumps(entry, sort_keys=True) + "\n")

    entries = load_history()
    write_dashboard(entries)

    print()
    print("== Benchmark snapshot recorded ==")
    print(f"History:   {HISTORY_FILE}")
    print(f"Raw log:   {raw_path}")
    print(f"Dashboard: {DASHBOARD_FILE}")
    print("Parsed median metrics:")
    for key in METRIC_KEYS:
        print(f"  {key}: {fmt_number(metrics[key])}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
