#include "market_data/MarketDataParser.hpp"
#include "market_data/OrderCommandParser.hpp"
#include "matching_engine/EngineEventPrinter.hpp"
#include "matching_engine/MatchingEngine.hpp"
#include "metrics/LatencyTracker.hpp"
#include "order_book/OrderBook.hpp"

#include <chrono>
#include <exception>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace {

void print_usage(const char* program) {
    std::cerr << "Usage:\n"
              << "  " << program << " --replay <market-events.csv>\n"
              << "  " << program << " --engine <order-commands.csv>\n";
}

void print_optional_price(const char* label, const std::optional<int64_t>& price) {
    std::cout << label;
    if (price) {
        std::cout << *price;
    } else {
        std::cout << "none";
    }
    std::cout << '\n';
}

int run_replay(const std::string& csv_path) {
    const auto events = market_data::MarketDataParser::parse_file(csv_path);

    order_book::OrderBook book;
    metrics::LatencyTracker tracker;

    for (const auto& event : events) {
        const auto start = std::chrono::steady_clock::now();
        book.apply_event(event);
        const auto end = std::chrono::steady_clock::now();
        const auto duration_ns = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count());
        tracker.record(duration_ns);
    }

    std::cout << "Replay mode: processed " << events.size() << " market events\n\n";

    print_optional_price("Best bid: ", book.best_bid());
    print_optional_price("Best ask: ", book.best_ask());
    print_optional_price("Spread: ", book.spread());
    std::cout << '\n';

    book.print_depth(std::cout, 5);
    std::cout << '\n';

    std::cout << "Latency:\n";
    std::cout << "avg: " << static_cast<uint64_t>(tracker.average()) << " ns\n";
    std::cout << "p50: " << tracker.p50() << " ns\n";
    std::cout << "p95: " << tracker.p95() << " ns\n";
    std::cout << "p99: " << tracker.p99() << " ns\n";

    return 0;
}

int run_engine_commands(const std::vector<matching_engine::OrderCommand>& commands) {
    matching_engine::MatchingEngine engine;
    metrics::LatencyTracker tracker;
    size_t trade_count = 0;

    for (const auto& command : commands) {
        const auto start = std::chrono::steady_clock::now();
        const auto events = engine.process(command);
        const auto end = std::chrono::steady_clock::now();
        const auto duration_ns = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count());
        tracker.record(duration_ns);

        matching_engine::print_engine_events(std::cout, events);
        for (const auto& event : events) {
            if (event.type == matching_engine::EngineEventType::Trade) {
                ++trade_count;
            }
        }
    }

    const auto& book = engine.book();
    std::cout << "\nEngine mode: processed " << commands.size() << " commands, " << trade_count
              << " trades\n\n";

    print_optional_price("Best bid: ", book.best_bid());
    print_optional_price("Best ask: ", book.best_ask());
    print_optional_price("Spread: ", book.spread());
    std::cout << '\n';

    book.print_depth(std::cout, 5);
    std::cout << '\n';

    std::cout << "Latency:\n";
    std::cout << "avg: " << static_cast<uint64_t>(tracker.average()) << " ns\n";
    std::cout << "min: " << tracker.min() << " ns\n";
    std::cout << "p50: " << tracker.p50() << " ns\n";
    std::cout << "p95: " << tracker.p95() << " ns\n";
    std::cout << "p99: " << tracker.p99() << " ns\n";
    std::cout << "max: " << tracker.max() << " ns\n";

    return 0;
}

int run_engine(const std::string& csv_path) {
    return run_engine_commands(market_data::OrderCommandParser::parse_file(csv_path));
}

}  // namespace

int main(int argc, char* argv[]) {
    try {
        if (argc == 2) {
            const std::string_view mode = argv[1];
            if (mode == "--replay" || mode == "--engine") {
                print_usage(argv[0]);
                return 1;
            }
        }

        if (argc == 3) {
            const std::string_view mode = argv[1];
            const std::string path = argv[2];
            if (mode == "--replay") {
                return run_replay(path);
            }
            if (mode == "--engine") {
                return run_engine(path);
            }
        }

        // Backward-compatible default: single path argument replays market events.
        if (argc == 2) {
            return run_replay(argv[1]);
        }

        print_usage(argv[0]);
        return 1;
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
}
