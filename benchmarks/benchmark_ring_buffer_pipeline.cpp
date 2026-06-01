#include "benchmarks/WorkloadGenerator.hpp"
#include "matching_engine/EngineEvent.hpp"
#include "matching_engine/MatchingEngine.hpp"
#include "pipeline/SpscCommandPipeline.hpp"

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

namespace {

size_t parse_size_arg(int argc, char* argv[], int index, size_t default_value) {
    if (argc > index) {
        return static_cast<size_t>(std::stoull(argv[index]));
    }
    return default_value;
}

size_t estimated_peak_active_orders(size_t command_count) {
    return command_count / 10;
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

bool events_equal(const std::vector<matching_engine::EngineEvent>& lhs,
                  const std::vector<matching_engine::EngineEvent>& rhs) {
    if (lhs.size() != rhs.size()) {
        return false;
    }
    for (size_t i = 0; i < lhs.size(); ++i) {
        const auto& a = lhs[i];
        const auto& b = rhs[i];
        if (a.type != b.type || a.order_id != b.order_id || a.reason != b.reason) {
            return false;
        }
        if (static_cast<bool>(a.trade) != static_cast<bool>(b.trade)) {
            return false;
        }
        if (a.trade) {
            if (!b.trade || a.trade->aggressive_order_id != b.trade->aggressive_order_id ||
                a.trade->resting_order_id != b.trade->resting_order_id ||
                a.trade->price != b.trade->price || a.trade->quantity != b.trade->quantity) {
                return false;
            }
        }
    }
    return true;
}

struct RunResult {
    double runtime_seconds{0.0};
    uint64_t throughput{0};
    uint64_t event_count{0};
    uint64_t trade_count{0};
    size_t active_orders{0};
    uint64_t resting_quantity{0};
    std::optional<int64_t> best_bid;
    std::optional<int64_t> best_ask;
    std::vector<matching_engine::EngineEvent> events;
};

void fill_book_metrics(RunResult& result, const matching_engine::MatchingEngine& engine) {
    const auto& book = engine.book();
    result.active_orders = book.active_order_count();
    result.resting_quantity = book.total_resting_quantity();
    result.best_bid = book.best_bid();
    result.best_ask = book.best_ask();
}

RunResult run_direct(const std::vector<matching_engine::OrderCommand>& commands) {
    matching_engine::MatchingEngine engine;
    engine.reserve_book_capacity(estimated_peak_active_orders(commands.size()));

    std::vector<matching_engine::EngineEvent> scratch;
    scratch.reserve(4);

    RunResult result;
    result.events.reserve(commands.size() * 2);

    const auto start = std::chrono::steady_clock::now();
    for (const auto& command : commands) {
        engine.process_into(command, scratch);
        result.events.insert(result.events.end(), scratch.begin(), scratch.end());
    }
    const auto end = std::chrono::steady_clock::now();

    result.runtime_seconds = std::chrono::duration<double>(end - start).count();
    result.throughput = static_cast<uint64_t>(static_cast<double>(commands.size()) / result.runtime_seconds);
    result.event_count = result.events.size();
    result.trade_count = count_trades(result.events);
    fill_book_metrics(result, engine);
    return result;
}

RunResult run_pipeline(const std::vector<matching_engine::OrderCommand>& commands,
                       size_t queue_capacity) {
    matching_engine::MatchingEngine engine;
    engine.reserve_book_capacity(estimated_peak_active_orders(commands.size()));
    pipeline::SpscCommandPipeline pipeline(queue_capacity);

    std::vector<matching_engine::EngineEvent> scratch;
    scratch.reserve(4);

    RunResult result;
    result.events.reserve(commands.size() * 2);

    const auto start = std::chrono::steady_clock::now();
    if (!pipeline.run_sequence(engine, commands, scratch, result.events)) {
        std::cerr << "Pipeline enqueue failed (queue capacity " << queue_capacity << ")\n";
        std::exit(1);
    }
    const auto end = std::chrono::steady_clock::now();

    result.runtime_seconds = std::chrono::duration<double>(end - start).count();
    result.throughput = static_cast<uint64_t>(static_cast<double>(commands.size()) / result.runtime_seconds);
    result.event_count = result.events.size();
    result.trade_count = count_trades(result.events);
    fill_book_metrics(result, engine);
    return result;
}

void print_optional_price(const char* label, const std::optional<int64_t>& price) {
    std::cout << label;
    if (price) {
        std::cout << *price;
    } else {
        std::cout << "n/a";
    }
    std::cout << '\n';
}

}  // namespace

int main(int argc, char* argv[]) {
    benchmarks::WorkloadConfig config;
    config.command_count = parse_size_arg(argc, argv, 1, 100'000);
    config.random_seed = static_cast<uint64_t>(parse_size_arg(argc, argv, 2, config.random_seed));

    const auto commands = benchmarks::WorkloadGenerator(config).generate();
    const size_t queue_capacity =
        argc > 3 ? parse_size_arg(argc, argv, 3, commands.size()) : commands.size();

    const auto direct = run_direct(commands);
    const auto pipeline = run_pipeline(commands, queue_capacity);

    if (!events_equal(direct.events, pipeline.events)) {
        std::cerr << "Sanity check failed: event sequences differ between direct and pipeline\n";
        return 1;
    }
    if (direct.trade_count != pipeline.trade_count ||
        direct.active_orders != pipeline.active_orders ||
        direct.resting_quantity != pipeline.resting_quantity || direct.best_bid != pipeline.best_bid ||
        direct.best_ask != pipeline.best_ask) {
        std::cerr << "Sanity check failed: final book metrics differ between direct and pipeline\n";
        return 1;
    }

    std::cout << "Benchmark: direct process_into vs SPSC command pipeline (local, machine-dependent)\n";
    std::cout << "Note: numbers below are not a claim that either path is faster in general.\n";
    std::cout << "Commands: " << commands.size() << '\n';
    std::cout << "Seed: " << config.random_seed << '\n';
    std::cout << "Pipeline queue capacity: " << queue_capacity << '\n';
    std::cout << '\n';

    std::cout << "Direct path:\n";
    std::cout << "  Runtime (s): " << direct.runtime_seconds << '\n';
    std::cout << "  Throughput (commands/sec): " << direct.throughput << '\n';
    std::cout << "  Events: " << direct.event_count << '\n';
    std::cout << "  Trades: " << direct.trade_count << '\n';
    std::cout << "  Active orders: " << direct.active_orders << '\n';
    std::cout << "  Resting quantity: " << direct.resting_quantity << '\n';
    print_optional_price("  Best bid: ", direct.best_bid);
    print_optional_price("  Best ask: ", direct.best_ask);
    std::cout << '\n';

    std::cout << "Pipeline path:\n";
    std::cout << "  Runtime (s): " << pipeline.runtime_seconds << '\n';
    std::cout << "  Throughput (commands/sec): " << pipeline.throughput << '\n';
    std::cout << "  Events: " << pipeline.event_count << '\n';
    std::cout << "  Trades: " << pipeline.trade_count << '\n';
    std::cout << "  Active orders: " << pipeline.active_orders << '\n';
    std::cout << "  Resting quantity: " << pipeline.resting_quantity << '\n';
    print_optional_price("  Best bid: ", pipeline.best_bid);
    print_optional_price("  Best ask: ", pipeline.best_ask);
    std::cout << '\n';

    std::cout << "Sanity: event sequence and final book metrics match between paths.\n";

    return 0;
}
