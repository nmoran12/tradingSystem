#include "benchmarks/WorkloadGenerator.hpp"
#include "matching_engine/MatchingEngine.hpp"
#include "metrics/LatencyTracker.hpp"

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>
#include <unistd.h>

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

size_t estimated_peak_active_orders(size_t command_count) {
    // Seed-42 synthetic workload: peak active resting orders ~= command_count / 10
    // (100k commands -> 10070 active; 1M -> 97187 in engine-only profiling).
    return command_count / 10;
}

bool has_flag(int argc, char* argv[], const std::string& flag) {
    for (int i = 1; i < argc; ++i) {
        if (argv[i] == flag) {
            return true;
        }
    }
    return false;
}

int run_engine_only_profile_mode(const benchmarks::WorkloadConfig& config) {
    const auto commands = benchmarks::WorkloadGenerator(config).generate();

    std::cout << "Benchmark: MatchingEngine engine-only profiling mode\n";
    std::cout << "Commands generated: " << commands.size() << '\n';
    std::cout << "Seed: " << config.random_seed << '\n';
    std::cout << "Workload generation complete; attach profiler before engine loop starts.\n";
    std::cout << "PID: " << getpid() << '\n';
    std::cout << "Suggested macOS sample command:\n";
    std::cout << "  sample " << getpid() << " 10 -file profiling/6e/matching_engine_engine_only_sample.txt\n";
    std::cout << "Engine loop starts in 10 seconds...\n" << std::flush;

    for (int seconds = 10; seconds > 0; --seconds) {
        std::cout << "  " << seconds << "...\n" << std::flush;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    matching_engine::MatchingEngine engine;
    engine.reserve_book_capacity(estimated_peak_active_orders(commands.size()));
    std::vector<matching_engine::EngineEvent> event_scratch;
    event_scratch.reserve(4);
    const auto runtime_start = std::chrono::steady_clock::now();
    for (const auto& command : commands) {
        engine.process_into(command, event_scratch);
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

    std::cout << "Engine-only loop complete\n";
    std::cout << "Runtime: " << runtime_seconds << "s\n";
    std::cout << "Throughput: " << static_cast<uint64_t>(throughput) << " commands/sec\n";
    std::cout << "Active orders: " << book.active_order_count() << '\n';
    std::cout << "Resting quantity: " << book.total_resting_quantity() << '\n';
    return 0;
}

}  // namespace

int main(int argc, char* argv[]) {
    benchmarks::WorkloadConfig config;
    config.command_count = parse_size_arg(argc, argv, 1, config.command_count);
    config.random_seed = static_cast<uint64_t>(parse_size_arg(argc, argv, 2, config.random_seed));

    if (has_flag(argc, argv, "--profile-engine-only")) {
        return run_engine_only_profile_mode(config);
    }

    const auto commands = benchmarks::WorkloadGenerator(config).generate();

    matching_engine::MatchingEngine engine;
    engine.reserve_book_capacity(estimated_peak_active_orders(commands.size()));
    metrics::LatencyTracker tracker;
    uint64_t total_trades = 0;
    std::vector<matching_engine::EngineEvent> event_scratch;
    event_scratch.reserve(4);

    const auto runtime_start = std::chrono::steady_clock::now();
    for (const auto& command : commands) {
        const auto start = std::chrono::steady_clock::now();
        engine.process_into(command, event_scratch);
        const auto end = std::chrono::steady_clock::now();
        tracker.record(static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count()));
        total_trades += count_trades(event_scratch);
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
