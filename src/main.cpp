#include "market_data/MarketDataParser.hpp"
#include "market_data/OrderCommandParser.hpp"
#include "matching_engine/EngineEventPrinter.hpp"
#include "matching_engine/MatchingEngine.hpp"
#include "metrics/LatencyTracker.hpp"
#include "order_book/OrderBook.hpp"
#include "protocol/BinaryCommandReader.hpp"

#include <chrono>
#include <exception>
#include <fstream>
#include <iostream>
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
              << " --binary-engine <order-commands.obk> --export-visualisation <replay.ndjson>\n";
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

std::string to_string(matching_engine::OrderCommandType type) {
    switch (type) {
        case matching_engine::OrderCommandType::NewOrder:
            return "new";
        case matching_engine::OrderCommandType::CancelOrder:
            return "cancel";
        case matching_engine::OrderCommandType::ModifyOrder:
            return "modify";
    }
    return "unknown";
}

std::string to_side_string(market_data::Side side) {
    switch (side) {
        case market_data::Side::BUY:
            return "buy";
        case market_data::Side::SELL:
            return "sell";
        case market_data::Side::UNKNOWN:
        default:
            return "unknown";
    }
}

std::string to_order_type_string(matching_engine::OrderType type) {
    switch (type) {
        case matching_engine::OrderType::Limit:
            return "limit";
        case matching_engine::OrderType::Market:
            return "market";
    }
    return "unknown";
}

void write_visualisation_step(std::ostream& out, std::size_t index,
                              const matching_engine::OrderCommand& command,
                              const std::vector<matching_engine::EngineEvent>& events,
                              const order_book::OrderBook& book) {
    const auto best_bid = book.best_bid();
    const auto best_ask = book.best_ask();
    const auto spread = book.spread();

    uint32_t best_bid_quantity = 0;
    if (best_bid) {
        best_bid_quantity =
            book.total_quantity_at_price(market_data::Side::BUY, *best_bid);
    }

    uint32_t best_ask_quantity = 0;
    if (best_ask) {
        best_ask_quantity =
            book.total_quantity_at_price(market_data::Side::SELL, *best_ask);
    }

    const auto active_orders = book.active_order_count();
    const auto resting_quantity = book.total_resting_quantity();

    out << '{';
    out << "\"schemaVersion\":1";
    out << ",\"index\":" << index;
    out << ",\"commandType\":\"" << to_string(command.type) << '"';
    out << ",\"side\":\"" << to_side_string(command.side) << '"';
    out << ",\"orderType\":\"" << to_order_type_string(command.order_type) << '"';
    out << ",\"orderId\":" << command.order_id;
    out << ",\"price\":" << command.price;
    out << ",\"quantity\":" << command.quantity;
    out << ",\"symbol\":\"" << command.symbol << '"';

    if (best_bid) {
        out << ",\"bestBid\":" << *best_bid;
    } else {
        out << ",\"bestBid\":null";
    }
    if (best_ask) {
        out << ",\"bestAsk\":" << *best_ask;
    } else {
        out << ",\"bestAsk\":null";
    }
    if (spread) {
        out << ",\"spread\":" << *spread;
    } else {
        out << ",\"spread\":null";
    }

    out << ",\"restingBidLevels\":[";
    if (best_bid && best_bid_quantity > 0) {
        out << "{\"price\":" << *best_bid << ",\"quantity\":" << best_bid_quantity
            << "}";
    }
    out << ']';

    out << ",\"restingAskLevels\":[";
    if (best_ask && best_ask_quantity > 0) {
        out << "{\"price\":" << *best_ask << ",\"quantity\":" << best_ask_quantity
            << "}";
    }
    out << ']';

    out << ",\"trades\":[";
    bool first_trade = true;
    for (const auto& event : events) {
        if (event.type != matching_engine::EngineEventType::Trade ||
            !event.trade) {
            continue;
        }
        if (!first_trade) {
            out << ',';
        }
        first_trade = false;
        const auto& trade = *event.trade;
        out << '{';
        out << "\"price\":" << trade.price;
        out << ",\"quantity\":" << trade.quantity;
        out << ",\"aggressiveOrderId\":" << trade.aggressive_order_id;
        out << ",\"restingOrderId\":" << trade.resting_order_id;
        out << '}';
    }
    out << ']';

    out << ",\"totalRestingOrders\":" << active_orders;
    out << ",\"totalRestingQuantity\":" << resting_quantity;

    out << "}\n";
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

int run_binary_engine(const std::string& binary_path,
                      const std::optional<std::string>& export_path) {
    const auto decoded_commands = protocol::read_order_commands_binary(binary_path);
    std::vector<matching_engine::OrderCommand> commands;
    commands.reserve(decoded_commands.size());
    for (const auto& decoded : decoded_commands) {
        commands.push_back(decoded.command);
    }
    if (!export_path) {
        return run_engine_commands(commands);
    }

    std::ofstream out(*export_path, std::ios::out | std::ios::trunc);
    if (!out) {
        throw std::runtime_error("Failed to open visualisation export file: " +
                                 *export_path);
    }

    matching_engine::MatchingEngine engine;
    metrics::LatencyTracker tracker;
    size_t trade_count = 0;

    for (std::size_t i = 0; i < commands.size(); ++i) {
        const auto& command = commands[i];
        const auto start = std::chrono::steady_clock::now();
        const auto events = engine.process(command);
        const auto end = std::chrono::steady_clock::now();
        const auto duration_ns = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(end - start)
                .count());
        tracker.record(duration_ns);

        matching_engine::print_engine_events(std::cout, events);
        for (const auto& event : events) {
            if (event.type == matching_engine::EngineEventType::Trade) {
                ++trade_count;
            }
        }

        write_visualisation_step(out, i, command, events, engine.book());
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

int run_binary_engine(const std::string& binary_path) {
    return run_binary_engine(binary_path, std::nullopt);
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
            if (mode == "--binary-engine" && flag == "--export-visualisation") {
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
            const std::string export_path = argv[4];

            if (mode == "--binary-engine" && flag == "--export-visualisation") {
                return run_binary_engine(path, export_path);
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
