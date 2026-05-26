#include "benchmarks/WorkloadGenerator.hpp"
#include "matching_engine/MatchingEngine.hpp"
#include "metrics/LatencyTracker.hpp"

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>

namespace {

size_t parse_size_arg(int argc, char* argv[], int index, size_t default_value) {
    if (argc > index) {
        return static_cast<size_t>(std::stoull(argv[index]));
    }
    return default_value;
}

uint64_t count_trades(const std::vector<matching_engine::EngineEvent>& events) {
    uint64_t trades = 0;
    for (const auto& event : events) {
        if (event.type == matching_engine::EngineEventType::Trade) {
            ++trades;
        }
    }
    return trades;
}

}  // namespace

int main(int argc, char* argv[]) {
    benchmarks::WorkloadConfig config;
    config.command_count = parse_size_arg(argc, argv, 1, config.command_count);
    config.random_seed = static_cast<uint64_t>(parse_size_arg(argc, argv, 2, config.random_seed));

    const auto commands = benchmarks::WorkloadGenerator(config).generate();

    matching_engine::MatchingEngine engine;
    metrics::LatencyTracker tracker;
    uint64_t total_trades = 0;

    const auto runtime_start = std::chrono::steady_clock::now();
    for (const auto& command : commands) {
        const auto start = std::chrono::steady_clock::now();
        const auto events = engine.process(command);
        const auto end = std::chrono::steady_clock::now();
        tracker.record(static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count()));
        total_trades += count_trades(events);
    }
    const auto runtime_end = std::chrono::steady_clock::now();

    const double runtime_seconds =
        std::chrono::duration<double>(runtime_end - runtime_start).count();
    const double throughput = static_cast<double>(commands.size()) / runtime_seconds;

    const auto& book = engine.book();
    if (const auto error = book.validate_invariants()) {
        std::cerr << "Invariant check failed: " << *error << '\n';
        return 1;
    }

    if (book.active_order_count() > commands.size()) {
        std::cerr << "Sanity check failed: active order count exceeds commands\n";
        return 1;
    }

    std::cout << "Benchmark: MatchingEngine synthetic workload\n";
    std::cout << "Commands: " << commands.size() << '\n';
    std::cout << "Seed: " << config.random_seed << '\n';
    std::cout << "Trades: " << total_trades << '\n';
    std::cout << "Runtime: " << runtime_seconds << "s\n";
    std::cout << "Throughput: " << static_cast<uint64_t>(throughput) << " commands/sec\n";
    std::cout << "Active orders: " << book.active_order_count() << '\n';
    std::cout << "Resting quantity: " << book.total_resting_quantity() << '\n';
    std::cout << "Latency ns:\n";
    std::cout << "  avg: " << static_cast<uint64_t>(tracker.average()) << '\n';
    std::cout << "  min: " << tracker.min() << '\n';
    std::cout << "  p50: " << tracker.p50() << '\n';
    std::cout << "  p95: " << tracker.p95() << '\n';
    std::cout << "  p99: " << tracker.p99() << '\n';
    std::cout << "  max: " << tracker.max() << '\n';

    return 0;
}
