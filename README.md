# cpp-low-latency-orderbook

A C++20 low-latency market data and order book engine that processes exchange-style events, reconstructs bid/ask depth, and benchmarks event processing latency.

This project focuses on core market data infrastructure: parsing simulated exchange messages, maintaining a price-time priority order book, matching client orders, and measuring per-event processing latency. It is not a trading bot and does not connect to live exchanges.

**Status:** Milestones 1–6 in progress · **123/123 tests** passing · replay + engine CLI + binary replay + benchmarks

Run all commands from this directory (`cpp-low-latency-orderbook/`), not the parent workspace folder.

## Project documentation

| Document | Description |
|----------|-------------|
| [docs/PROJECT_OVERVIEW.md](docs/PROJECT_OVERVIEW.md) | Goals, components, portfolio value |
| [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) | Module boundaries, data flow, invariants |
| [docs/ROADMAP.md](docs/ROADMAP.md) | Completed and future milestones |
| [docs/DEVELOPMENT_RULES.md](docs/DEVELOPMENT_RULES.md) | Coding, testing, and architecture guardrails |
| [docs/BENCHMARKING.md](docs/BENCHMARKING.md) | Benchmark harness and sample results |
| [docs/PERFORMANCE_BASELINE.md](docs/PERFORMANCE_BASELINE.md) | Release-mode baseline methodology |
| [docs/PROFILING_REPORT.md](docs/PROFILING_REPORT.md) | Profiling tools and benchmark stability |
| [docs/MILESTONE_5_PLAN.md](docs/MILESTONE_5_PLAN.md) | Binary protocol milestone plan |
| [docs/MILESTONE_6_PLAN.md](docs/MILESTONE_6_PLAN.md) | Performance and SPSC milestone plan |

## Architecture

**Replay path (Milestone 1):**

```
Market Data CSV → MarketDataParser → MarketEvent → OrderBook → Depth / Latency
```

**Engine path (Milestones 2–3):**

```
Command CSV → OrderCommandParser → OrderCommand → MatchingEngine → EngineEvent → OrderBook
```

## Build

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Requirements:

- C++20 compiler (GCC 11+, Clang 14+, or Apple Clang 14+)
- CMake 3.20+
- Internet access on first configure (GoogleTest is fetched via CMake FetchContent)

## Run

From project root (after `cmake --build build`):

**Replay mode** (Milestone 1 market-event CSV):

```bash
./build/cpp-low-latency-orderbook --replay data/sample_events.csv
# backward compatible:
./build/cpp-low-latency-orderbook data/sample_events.csv
```

**Engine mode** (Milestone 3 command CSV through MatchingEngine):

```bash
./build/cpp-low-latency-orderbook --engine data/sample_commands.csv
```

**Binary engine mode** (Milestone 5 OBK1 command file through MatchingEngine):

```bash
./build/cpp-low-latency-orderbook --binary-engine path/to/order_commands.obk
```

Binary files contain back-to-back fixed-width 64-byte OBK1 v1 command messages with no file header. Use `protocol::write_order_commands_binary` from tests or tooling to generate local simulated command files. Protocol helpers support both buffered and streaming binary reads; this is still simulated/local replay only, not real exchange connectivity.

Engine modes print each `EngineEvent`, final best bid/ask, book depth, and latency stats.

### Command CSV format

```csv
type,order_id,side,order_type,price,quantity
NEW,1,SELL,LIMIT,10055,100
NEW,2,BUY,LIMIT,10060,150
NEW,3,SELL,LIMIT,10070,50
NEW,4,BUY,MARKET,0,25
CANCEL,2,BUY,LIMIT,0,0
MODIFY,1,SELL,LIMIT,10050,75
```

- `type`: `NEW`, `CANCEL`, or `MODIFY`
- `price` / `quantity`: integer ticks and shares; use `0` for market price placeholder
- Prices are integer cents/ticks (e.g. `10055` = $100.55)

**Sample engine run** (`data/sample_commands.csv`):

1. Resting sell at 10055 (order 1)
2. Aggressive buy limit 10060 (order 2) fully trades against order 1; remainder rests
3. Additional ask at 10070 (order 3)
4. Market buy 25 (order 4) consumes available ask liquidity

## Milestone 3: Exchange Command Layer

- **Market orders** — consume liquidity price-time priority; unfilled quantity is cancelled (never rests)
- **Modify orders** — cancel-and-reinsert by `order_id`; crossing price triggers normal matching
- **OrderCommandParser** — separate CSV parser for engine commands
- **Engine CLI** — `--engine` mode with event printing
- **OrderBook introspection** — `get_order`, `active_order_count`, `total_resting_quantity`, `validate_invariants`

**Modify tradeoff:** all successful modifies use cancel-and-reinsert, so FIFO queue position is always reset.

## Milestone 4: Benchmarking

See [docs/BENCHMARKING.md](docs/BENCHMARKING.md) for full methodology and **machine-dependent sample results** (not production claims).

```bash
./build/matching_engine_benchmark              # default 1M commands, seed 42
./build/matching_engine_benchmark 100000 42    # custom size/seed
./build/binary_protocol_benchmark 100000 42    # OBK1 write/read/decode + engine apply phases
```

Release build recommended for meaningful throughput: `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release`

For repeatable local baselines, use:

```bash
./scripts/benchmark_release.sh 100000 42
```

Benchmark numbers are machine-specific. See [docs/PERFORMANCE_BASELINE.md](docs/PERFORMANCE_BASELINE.md) for the current Release-mode baseline workflow and recorded local results. For repeated runs and profiling guidance, see [docs/PROFILING_REPORT.md](docs/PROFILING_REPORT.md).

## Current Features

- Market-event CSV replay (`ADD` / `CANCEL` / `EXECUTE`)
- Command CSV exchange simulator (`NEW` / `CANCEL` / `MODIFY`, `LIMIT` / `MARKET`)
- Binary OBK1 command-file replay via `--binary-engine`
- MatchingEngine with automatic trade generation, partial/full fills, cancels
- Latency tracker with min/max/average and p50/p95/p99
- GoogleTest coverage + `matching_engine_benchmark` + `binary_protocol_benchmark`

## Project Layout

```
include/
  market_data/      MarketDataParser, OrderCommandParser
  order_book/       Order, Trade, OrderBook
  matching_engine/  OrderCommand, EngineEvent, MatchingEngine, EngineEventPrinter
  benchmarks/       WorkloadGenerator
  metrics/          LatencyTracker
  protocol/         OBK1 binary encode/decode and command file reader/writer
benchmarks/         matching_engine_benchmark.cpp
                    benchmark_binary_protocol.cpp
data/               sample_events.csv, sample_commands.csv
tests/
```

## Future milestones

See [docs/ROADMAP.md](docs/ROADMAP.md). Current performance work is tracked in **Milestone 6**.

## License

Educational / portfolio use. No warranty. Not for production trading.
