# Demo guide

Step-by-step instructions to build, test, and demonstrate `cpp-low-latency-orderbook` from a fresh clone. For the **live SSE visualiser demo**, use `./scripts/demo-live-replay.sh` (stable repo-local `.obk`, no temp-file copy step).

**Related:** [README.md](../README.md) · [REPLAY_VISUALISER.md](REPLAY_VISUALISER.md) · [BINARY_PROTOCOL.md](BINARY_PROTOCOL.md)

---

## Quick path: clone → build → test → demo

Run from the repository root (`cpp-low-latency-orderbook/`).

```bash
# 1. Clone (replace with your fork URL if needed)
git clone https://github.com/nmoran12/tradingSystem.git
cd tradingSystem/cpp-low-latency-orderbook   # or your clone path

# 2. Configure and build
cmake -S . -B build
cmake --build build

# 3. Test (160 GoogleTest cases)
./scripts/verify.sh

# 4a. Demo without UI — CSV engine
./build/cpp-low-latency-orderbook --engine data/sample_commands.csv

# 4b. Demo without UI — market replay
./build/cpp-low-latency-orderbook --replay data/sample_events.csv

# 4c. Demo with UI — offline bundled scenario (no .obk required)
cd ui/replay-visualiser
npm install
npm run dev
# Open the dev URL; choose a bundled scenario (e.g. sample-replay)

# 4d. Demo with UI — live SSE stream (recommended)
# Terminal 1:
./scripts/demo-live-replay.sh
# Terminal 2:
cd ui/replay-visualiser && npm run dev
# Connect to http://127.0.0.1:9000/stream
```

**Optional UI production build:** `npm run build` (output in `ui/replay-visualiser/dist/`).

---

## Generate a local `.obk` file (OBK1)

The repo does **not** commit `.obk` command files (`*.obk` and `tmp/` are gitignored). For binary-engine, export, or live-stream demos you need a local file.

**Recommended for demos (stable path under the repo):**

```bash
cmake -S . -B build && cmake --build build --target write_demo_obk
./build/write_demo_obk tmp/demo/my_demo.obk 5000 42
```

Or let `./scripts/demo-live-replay.sh` create `tmp/demo/live_demo_5000_42.obk` automatically.

**Do not use `binary_protocol_benchmark` output for demos.** It writes under the system temp directory (e.g. `/var/folders/...`) and **deletes the file when the benchmark exits**, so the path printed mid-run often disappears before you can use it.

**Benchmark-only** (throughput measurement, not demo input):

```bash
./build/binary_protocol_benchmark 5000 42
```

**Alternative:** `write_demo_obk` or GoogleTest binary writer tests. See [BINARY_PROTOCOL.md](BINARY_PROTOCOL.md).

---

## Offline visualiser demo (bundled scenarios)

No C++ binary file required.

| Step | Command / action |
|------|------------------|
| Build UI | `cd ui/replay-visualiser && npm install && npm run dev` |
| Open browser | Use the Vite dev server URL (printed in the terminal) |
| Load scenario | Pick **sample-replay**, **sample-crossing-trades**, **sample-spread-movement**, or **sample-deep-book** from the UI |

### Fixture honesty

| Asset | What it is |
|-------|------------|
| `ui/replay-visualiser/public/sample-*.ndjson` | **Hand-authored demo fixtures** for the UI. Some show **multiple price levels** per side to exercise the ladder. |
| NDJSON from `--export-visualisation` | **Real C++ exporter output** — **shallow depth only** (best bid / best ask per side, one aggregated level each). |

When validating export behaviour, always compare against CLI-generated NDJSON, not the richer bundled files. Details: [REPLAY_VISUALISER.md](REPLAY_VISUALISER.md) §Bundled UI samples.

---

## Export NDJSON from binary replay

```bash
./build/write_demo_obk tmp/demo/export_demo.obk 5000 42
OBK=tmp/demo/export_demo.obk

./build/cpp-low-latency-orderbook \
  --binary-engine "$OBK" \
  --export-visualisation /tmp/replay.ndjson

cd ui/replay-visualiser && npm run dev
# Use the file picker to load /tmp/replay.ndjson
```

Each line is one `schemaVersion: 1` step (command + shallow book + trades). See [REPLAY_VISUALISER.md](REPLAY_VISUALISER.md) for the field list.

---

## Live SSE visualisation demo

**Order matters:** start the C++ stream server first, then connect the UI (or `curl`).

**Terminal 1 — backend** (builds if needed, writes a stable OBK under `tmp/demo/`, starts the server):

```bash
./scripts/demo-live-replay.sh
```

Options: `--force` (regenerate `.obk`), `--commands N`, `--seed N`. Default file: `tmp/demo/live_demo_5000_42.obk`. Default pacing: `STREAM_DELAY_MS=10` between streamed steps (override, e.g. `STREAM_DELAY_MS=20 ./scripts/demo-live-replay.sh`).

**Terminal 2 — UI:**

```bash
cd ui/replay-visualiser
npm install
npm run dev
```

In the **Live stream** section: URL `http://127.0.0.1:9000/stream` → **Connect**. Steps append as replay runs; **Run summary** updates as steps arrive (client-side metrics only — not Release benchmarks).

**Manual equivalent** (if you prefer not to use the script):

```bash
./build/write_demo_obk tmp/demo/live_demo_5000_42.obk 5000 42
./build/cpp-low-latency-orderbook \
  --binary-engine tmp/demo/live_demo_5000_42.obk \
  --stream-visualisation 127.0.0.1:9000 \
  --stream-delay-ms 10
```

**Terminal 2 alternative — debug without UI:**

```bash
curl -N http://127.0.0.1:9000/stream
```

Limitations: localhost-only, single client, no TLS/auth — proof-of-concept only. [REPLAY_VISUALISER.md](REPLAY_VISUALISER.md)

---

## Screenshot / GIF capture checklist (optional assets)

Screenshots and GIFs are **not required** in the repository for the project to function. Use this checklist when preparing a GitHub README or portfolio page.

| # | Capture | Suggested content | Save as (example) |
|---|---------|-------------------|-------------------|
| 1 | UI — offline scenario | Ladder + trade tape mid-replay | `docs/images/demo-offline.png` |
| 2 | UI — run summary | Run summary panel with totals / final BBO | `docs/images/demo-run-summary.png` |
| 3 | UI — live mode | Live stream connected, steps appending | `docs/images/demo-live-sse.png` |
| 4 | Terminal — verify | `./scripts/verify.sh` showing 160/160 passed | `docs/images/demo-verify.png` |
| 5 | GIF (optional) | Short play-through of sample-crossing-trades | `docs/images/demo-replay.gif` |

**Before committing images:**

- [ ] Add `docs/images/` paths to README only **after** files exist (do not claim assets that are missing).
- [ ] Redact machine-specific paths if desired.
- [ ] Keep file sizes reasonable for git (prefer PNG; compress GIFs).

**Live demo script:** `./scripts/demo-live-replay.sh` — see [Live SSE visualisation demo](#live-sse-visualisation-demo) above.

---

## Repository hygiene

| Path | Policy |
|------|--------|
| `profiling/` | Local Instruments / trace output — **keep untracked**; do not commit |
| `ui/replay-visualiser/node_modules/`, `dist/` | Do not commit |
| `*.obk` | Generated locally; typically gitignored |
| `build/` | Local CMake output |

**CI:** GitHub Actions ([`../.github/workflows/ci.yml`](../.github/workflows/ci.yml)) runs on push/PR: C++ build + `ctest` (164 tests) and `ui/replay-visualiser` production build on Ubuntu.

**Not implemented:** TCP order gateway, market data publisher, persistence/replay log — candidates only ([ROADMAP.md](ROADMAP.md)).
