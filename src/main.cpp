#include "market_data/MarketDataParser.hpp"
#include "market_data/OrderCommandParser.hpp"
#include "matching_engine/EngineEventPrinter.hpp"
#include "matching_engine/MatchingEngine.hpp"
#include "metrics/LatencyTracker.hpp"
#include "order_book/OrderBook.hpp"
#include "protocol/BinaryCommandReader.hpp"
#include "viz/ReplayVisualisationStreamServer.hpp"
#include "viz/ReplayVisualisationWriter.hpp"

#include <chrono>
#include <exception>
#include <fstream>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace {

void print_usage(const char* program) {
    std::cerr << "Usage:\n"
              << "  " << program << " --replay <market-events.csv>\n"
              << "  " << program << " --engine <order-commands.csv>\n"
              << "  " << program << " --binary-engine <order-commands.obk>\n"
              << "  " << program
              << " --binary-engine <order-commands.obk> --export-visualisation <replay.ndjson>\n"
              << "  " << program
              << " --binary-engine <order-commands.obk> --stream-visualisation <host:port>\n";
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

    std::vector<matching_engine::EngineEvent> event_scratch;
    event_scratch.reserve(4);
    for (const auto& command : commands) {
        const auto start = std::chrono::steady_clock::now();
        engine.process_into(command, event_scratch);
        const auto end = std::chrono::steady_clock::now();
        const auto duration_ns = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count());
        tracker.record(duration_ns);

        matching_engine::print_engine_events(std::cout, event_scratch);
        for (const auto& event : event_scratch) {
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

enum class BinaryVisualisationMode { None, FileExport, Stream };

int run_binary_engine_with_visualisation(
    const std::string& binary_path, BinaryVisualisationMode mode,
    const std::optional<std::string>& export_path,
    const std::optional<std::string>& stream_endpoint) {
    const auto decoded_commands = protocol::read_order_commands_binary(binary_path);
    std::vector<matching_engine::OrderCommand> commands;
    commands.reserve(decoded_commands.size());
    for (const auto& decoded : decoded_commands) {
        commands.push_back(decoded.command);
    }

    std::ofstream export_file;
    if (mode == BinaryVisualisationMode::FileExport) {
        if (!export_path) {
            throw std::runtime_error("export path required for file visualisation mode");
        }
        export_file.open(*export_path, std::ios::out | std::ios::trunc);
        if (!export_file) {
            throw std::runtime_error("Failed to open visualisation export file: " +
                                     *export_path);
        }
    }

    std::unique_ptr<viz::ReplayVisualisationStreamServer> stream_server;
    if (mode == BinaryVisualisationMode::Stream) {
        if (!stream_endpoint) {
            throw std::runtime_error("stream endpoint required for stream visualisation mode");
        }
        stream_server =
            std::make_unique<viz::ReplayVisualisationStreamServer>(*stream_endpoint);
        stream_server->wait_for_client();
    }

    matching_engine::MatchingEngine engine;
    metrics::LatencyTracker tracker;
    size_t trade_count = 0;

    std::vector<matching_engine::EngineEvent> event_scratch;
    event_scratch.reserve(4);
    for (std::size_t i = 0; i < commands.size(); ++i) {
        const auto& command = commands[i];
        const auto start = std::chrono::steady_clock::now();
        engine.process_into(command, event_scratch);
        const auto end = std::chrono::steady_clock::now();
        const auto duration_ns = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count());
        tracker.record(duration_ns);

        matching_engine::print_engine_events(std::cout, event_scratch);
        for (const auto& event : event_scratch) {
            if (event.type == matching_engine::EngineEventType::Trade) {
                ++trade_count;
            }
        }

        if (mode == BinaryVisualisationMode::FileExport) {
            viz::ReplayVisualisationWriter::write_ndjson_line(export_file, i, command,
                                                              event_scratch, engine.book());
        } else if (mode == BinaryVisualisationMode::Stream) {
            stream_server->send_sse_record(viz::ReplayVisualisationWriter::format_record(
                i, command, event_scratch, engine.book()));
        }
    }

    if (stream_server) {
        stream_server->close();
    }

    const auto& book = engine.book();
    std::cout << "\nEngine mode: processed " << commands.size() << " commands, "
              << trade_count << " trades\n\n";

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

int run_binary_engine(const std::string& binary_path,
                      const std::optional<std::string>& export_path) {
    return run_binary_engine_with_visualisation(
        binary_path, BinaryVisualisationMode::FileExport, export_path, std::nullopt);
}

int run_binary_engine_stream(const std::string& binary_path,
                             const std::string& stream_endpoint) {
    return run_binary_engine_with_visualisation(
        binary_path, BinaryVisualisationMode::Stream, std::nullopt, stream_endpoint);
}

int run_binary_engine(const std::string& binary_path) {
    return run_binary_engine_with_visualisation(binary_path, BinaryVisualisationMode::None,
                                                std::nullopt, std::nullopt);
}

}  // namespace

int main(int argc, char* argv[]) {
    try {
        if (argc == 2) {
            const std::string_view mode = argv[1];
            if (mode == "--replay" || mode == "--engine" || mode == "--binary-engine") {
                print_usage(argv[0]);
                return 1;
            }
        }

        if (argc == 4) {
            const std::string_view mode = argv[1];
            const std::string_view flag = argv[3];
            if (mode == "--binary-engine" &&
                (flag == "--export-visualisation" || flag == "--stream-visualisation")) {
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
            if (mode == "--binary-engine") {
                return run_binary_engine(path);
            }
        }

        if (argc == 5) {
            const std::string_view mode = argv[1];
            const std::string path = argv[2];
            const std::string_view flag = argv[3];
            const std::string option_value = argv[4];

            if (mode == "--binary-engine" && flag == "--export-visualisation") {
                return run_binary_engine(path, option_value);
            }
            if (mode == "--binary-engine" && flag == "--stream-visualisation") {
                return run_binary_engine_stream(path, option_value);
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
