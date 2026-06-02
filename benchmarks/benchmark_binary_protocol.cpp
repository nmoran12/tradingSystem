#include "benchmarks/WorkloadGenerator.hpp"
#include "matching_engine/EngineEvent.hpp"
#include "matching_engine/MatchingEngine.hpp"
#include "protocol/BinaryCommandReader.hpp"
#include "protocol/BinaryProtocol.hpp"

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace {

struct PhaseTiming {
    double seconds{};
    uint64_t nanoseconds{};
};

template <typename Fn>
PhaseTiming time_phase(Fn&& fn) {
    const auto start = std::chrono::steady_clock::now();
    fn();
    const auto end = std::chrono::steady_clock::now();
    const auto nanoseconds =
        static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(end - start)
                                  .count());
    return {std::chrono::duration<double>(end - start).count(), nanoseconds};
}

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

double commands_per_second(size_t command_count, double seconds) {
    if (seconds == 0.0) {
        return 0.0;
    }
    return static_cast<double>(command_count) / seconds;
}

uint64_t avg_ns_per_command(size_t command_count, uint64_t nanoseconds) {
    if (command_count == 0) {
        return 0;
    }
    return nanoseconds / command_count;
}

int64_t signed_difference(uint64_t lhs, uint64_t rhs) {
    return static_cast<int64_t>(lhs) - static_cast<int64_t>(rhs);
}

std::vector<protocol::DecodedOrderCommand> attach_timestamps(
    std::vector<matching_engine::OrderCommand>& commands) {
    std::vector<protocol::DecodedOrderCommand> decoded;
    decoded.reserve(commands.size());
    uint64_t timestamp = 1;
    for (auto& command : commands) {
        decoded.push_back({std::move(command), timestamp++});
    }
    return decoded;
}

std::filesystem::path benchmark_file_path(uint64_t seed) {
    return std::filesystem::temp_directory_path() /
           ("cpp_low_latency_orderbook_binary_protocol_" + std::to_string(seed) + ".obk");
}

}  // namespace

int main(int argc, char* argv[]) {
    try {
        benchmarks::WorkloadConfig config;
        config.command_count = parse_size_arg(argc, argv, 1, 100'000);
        config.random_seed = static_cast<uint64_t>(parse_size_arg(argc, argv, 2, 42));

        std::vector<matching_engine::OrderCommand> commands;
        const auto generation_timing = time_phase([&] {
            commands = benchmarks::WorkloadGenerator(config).generate();
        });

        const auto decoded_for_write = attach_timestamps(commands);
        const auto path = benchmark_file_path(config.random_seed);
        std::filesystem::remove(path);

        const auto write_timing = time_phase([&] {
            protocol::write_order_commands_binary(path, decoded_for_write);
        });

        const auto binary_file_size = std::filesystem::file_size(path);

        std::vector<protocol::DecodedOrderCommand> decoded_from_file;
        const auto read_decode_timing = time_phase([&] {
            decoded_from_file = protocol::read_order_commands_binary(path);
        });

        matching_engine::MatchingEngine buffered_engine;
        buffered_engine.reserve_book_capacity(commands.size() / 10);
        uint64_t buffered_trades = 0;
        std::vector<matching_engine::EngineEvent> buffered_event_scratch;
        buffered_event_scratch.reserve(4);
        const auto buffered_engine_apply_timing = time_phase([&] {
            for (const auto& decoded : decoded_from_file) {
                buffered_engine.process_into(decoded.command, buffered_event_scratch);
                buffered_trades += count_trades(buffered_event_scratch);
            }
        });

        const auto& buffered_book = buffered_engine.book();
        if (const auto error = buffered_book.validate_invariants()) {
            std::cerr << "Buffered invariant check failed: " << *error << '\n';
            return 1;
        }

        matching_engine::MatchingEngine streaming_engine;
        streaming_engine.reserve_book_capacity(commands.size() / 10);
        uint64_t streaming_trades = 0;
        std::size_t streaming_commands = 0;
        std::vector<matching_engine::EngineEvent> streaming_event_scratch;
        streaming_event_scratch.reserve(4);
        const auto streaming_timing = time_phase([&] {
            streaming_commands = protocol::stream_order_commands_binary(
                path, [&](const protocol::DecodedOrderCommand& decoded) {
                    streaming_engine.process_into(decoded.command, streaming_event_scratch);
                    streaming_trades += count_trades(streaming_event_scratch);
                });
        });

        const auto& streaming_book = streaming_engine.book();
        if (const auto error = streaming_book.validate_invariants()) {
            std::cerr << "Streaming invariant check failed: " << *error << '\n';
            return 1;
        }
        if (streaming_commands != commands.size()) {
            std::cerr << "Streaming sanity check failed: command count mismatch\n";
            return 1;
        }

        const auto buffered_total_ns =
            read_decode_timing.nanoseconds + buffered_engine_apply_timing.nanoseconds;
        const auto streaming_total_ns = streaming_timing.nanoseconds;
        const auto buffered_vs_streaming_ns =
            signed_difference(buffered_total_ns, streaming_total_ns);

        const auto total_ns = generation_timing.nanoseconds + write_timing.nanoseconds +
                              read_decode_timing.nanoseconds +
                              buffered_engine_apply_timing.nanoseconds +
                              streaming_timing.nanoseconds;
        const double total_seconds = generation_timing.seconds + write_timing.seconds +
                                     read_decode_timing.seconds +
                                     buffered_engine_apply_timing.seconds +
                                     streaming_timing.seconds;

        std::cout << "Benchmark: Binary protocol replay\n";
        std::cout << "Commands: " << commands.size() << '\n';
        std::cout << "Seed: " << config.random_seed << '\n';
        std::cout << "Binary file: " << path << '\n';
        std::cout << "Binary file size: " << binary_file_size << " bytes\n";
        std::cout << "Buffered trades: " << buffered_trades << '\n';
        std::cout << "Streaming trades: " << streaming_trades << '\n';
        std::cout << "Buffered active orders: " << buffered_book.active_order_count() << '\n';
        std::cout << "Streaming active orders: " << streaming_book.active_order_count() << '\n';
        std::cout << "Buffered resting quantity: " << buffered_book.total_resting_quantity()
                  << '\n';
        std::cout << "Streaming resting quantity: " << streaming_book.total_resting_quantity()
                  << '\n';
        std::cout << "Phases:\n";
        std::cout << "  command generation: " << generation_timing.seconds << "s\n";
        std::cout << "  binary write: " << write_timing.seconds << "s ("
                  << static_cast<uint64_t>(commands_per_second(commands.size(),
                                                               write_timing.seconds))
                  << " commands/sec, "
                  << avg_ns_per_command(commands.size(), write_timing.nanoseconds)
                  << " ns/command)\n";
        std::cout << "  buffered read/decode: " << read_decode_timing.seconds << "s ("
                  << static_cast<uint64_t>(commands_per_second(commands.size(),
                                                               read_decode_timing.seconds))
                  << " commands/sec, "
                  << avg_ns_per_command(commands.size(), read_decode_timing.nanoseconds)
                  << " ns/command)\n";
        std::cout << "  buffered engine apply: " << buffered_engine_apply_timing.seconds << "s ("
                  << static_cast<uint64_t>(commands_per_second(commands.size(),
                                                               buffered_engine_apply_timing.seconds))
                  << " commands/sec, "
                  << avg_ns_per_command(commands.size(),
                                        buffered_engine_apply_timing.nanoseconds)
                  << " ns/command)\n";
        std::cout << "  buffered read/decode + apply total: "
                  << (read_decode_timing.seconds + buffered_engine_apply_timing.seconds)
                  << "s (" << buffered_total_ns << " ns)\n";
        std::cout << "  streaming read/decode/apply: " << streaming_timing.seconds << "s ("
                  << static_cast<uint64_t>(commands_per_second(streaming_commands,
                                                               streaming_timing.seconds))
                  << " commands/sec, "
                  << avg_ns_per_command(streaming_commands, streaming_timing.nanoseconds)
                  << " ns/command)\n";
        std::cout << "  buffered total minus streaming total: " << buffered_vs_streaming_ns
                  << " ns\n";
        std::cout << "  total measured phases: " << total_seconds << "s (" << total_ns
                  << " ns)\n";
        std::cout << "Note: metrics are local wall-clock timings; p50/p95/p99 are not reported "
                     "because this benchmark measures phase totals, not per-command decode "
                     "latencies. Positive buffered-minus-streaming means streaming was faster "
                     "on this run; negative means buffered was faster.\n";
        std::cout << "Note: this benchmark deletes the temp binary file when it exits. "
                     "For demos, use ./scripts/demo-live-replay.sh (stable file under "
                     "tmp/demo/).\n";

        std::error_code ignored;
        std::filesystem::remove(path, ignored);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Benchmark failed: " << error.what() << '\n';
        return 1;
    }
}
