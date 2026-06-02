#include "benchmarks/WorkloadGenerator.hpp"
#include "protocol/BinaryCommandReader.hpp"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace {

size_t parse_size_arg(int argc, char* argv[], int index, size_t default_value) {
    if (argc > index) {
        return static_cast<size_t>(std::stoull(argv[index]));
    }
    return default_value;
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

void print_usage(const char* program) {
    std::cerr << "Usage: " << program << " <output.obk> [command_count] [seed]\n"
              << "  Writes a stable OBK1 demo command file (not deleted after write).\n"
              << "  Defaults: command_count=5000, seed=42\n";
}

}  // namespace

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    try {
        const std::filesystem::path output_path = argv[1];
        benchmarks::WorkloadConfig config;
        config.command_count = parse_size_arg(argc, argv, 2, 5'000);
        config.random_seed = static_cast<uint64_t>(parse_size_arg(argc, argv, 3, 42));

        if (output_path.empty()) {
            std::cerr << "Output path must not be empty\n";
            return 1;
        }

        const auto parent = output_path.parent_path();
        if (!parent.empty()) {
            std::filesystem::create_directories(parent);
        }

        auto commands = benchmarks::WorkloadGenerator(config).generate();
        const auto decoded = attach_timestamps(commands);
        protocol::write_order_commands_binary(output_path, decoded);

        const auto file_size = std::filesystem::file_size(output_path);
        std::cout << "Wrote demo OBK1 file: " << output_path << '\n';
        std::cout << "Commands: " << decoded.size() << '\n';
        std::cout << "Seed: " << config.random_seed << '\n';
        std::cout << "File size: " << file_size << " bytes\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "write_demo_obk failed: " << error.what() << '\n';
        return 1;
    }
}
