#include "benchmarks/WorkloadGenerator.hpp"
#include "protocol/BinaryCommandReader.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <string>
#include <vector>

using namespace benchmarks;

namespace {

std::vector<protocol::DecodedOrderCommand> attach_timestamps(
    const std::vector<matching_engine::OrderCommand>& commands) {
    std::vector<protocol::DecodedOrderCommand> decoded;
    decoded.reserve(commands.size());
    uint64_t timestamp = 1;
    for (const auto& command : commands) {
        decoded.push_back({command, timestamp++});
    }
    return decoded;
}

std::filesystem::path temp_file_path() {
    const auto* test_info = ::testing::UnitTest::GetInstance()->current_test_info();
    return std::filesystem::temp_directory_path() /
           ("orderbook_" + std::string(test_info->test_suite_name()) + "_" +
            test_info->name() + ".obk");
}

class ScopedTempFile {
public:
    ScopedTempFile() : path_(temp_file_path()) {
        std::filesystem::remove(path_);
    }

    ~ScopedTempFile() {
        std::error_code ignored;
        std::filesystem::remove(path_, ignored);
    }

    const std::filesystem::path& path() const {
        return path_;
    }

private:
    std::filesystem::path path_;
};

}  // namespace

TEST(WorkloadGeneratorTest, GeneratesDeterministicCommandsForFixedSeed) {
    WorkloadConfig config;
    config.command_count = 500;
    config.random_seed = 42;

    const auto first = WorkloadGenerator(config).generate();
    const auto second = WorkloadGenerator(config).generate();

    ASSERT_EQ(first.size(), second.size());
    for (size_t i = 0; i < first.size(); ++i) {
        EXPECT_EQ(first[i].type, second[i].type);
        EXPECT_EQ(first[i].order_id, second[i].order_id);
        EXPECT_EQ(first[i].side, second[i].side);
        EXPECT_EQ(first[i].order_type, second[i].order_type);
        EXPECT_EQ(first[i].price, second[i].price);
        EXPECT_EQ(first[i].quantity, second[i].quantity);
        EXPECT_EQ(first[i].symbol, second[i].symbol);
    }
}

TEST(WorkloadGeneratorTest, RejectsInvalidMix) {
    WorkloadConfig config;
    config.limit_order_pct = 0.5;
    config.market_order_pct = 0.2;
    config.cancel_pct = 0.1;
    config.modify_pct = 0.1;
    EXPECT_THROW((void)WorkloadGenerator(config), std::invalid_argument);
}

TEST(WorkloadGeneratorTest, GeneratesRequestedCommandCount) {
    WorkloadConfig config;
    config.command_count = 123;

    const auto commands = WorkloadGenerator(config).generate();

    EXPECT_EQ(commands.size(), config.command_count);
}

TEST(WorkloadGeneratorTest, AssignsSymbolsFromConfiguredSet) {
    WorkloadConfig config;
    config.command_count = 200;
    config.symbols = {"AAPL", "MSFT", "NVDA"};

    const auto commands = WorkloadGenerator(config).generate();

    for (const auto& command : commands) {
        EXPECT_TRUE(command.symbol == "AAPL" || command.symbol == "MSFT" ||
                    command.symbol == "NVDA");
    }
}

TEST(WorkloadGeneratorTest, RejectsZeroCommandCount) {
    WorkloadConfig config;
    config.command_count = 0;

    EXPECT_THROW((void)WorkloadGenerator(config), std::invalid_argument);
}

TEST(WorkloadGeneratorTest, GeneratedCommandsRoundTripThroughBinaryFile) {
    WorkloadConfig config;
    config.command_count = 50;
    config.random_seed = 7;
    ScopedTempFile temp_file;

    const auto commands = WorkloadGenerator(config).generate();
    const auto decoded_for_write = attach_timestamps(commands);
    protocol::write_order_commands_binary(temp_file.path(), decoded_for_write);
    const auto read_back = protocol::read_order_commands_binary(temp_file.path());

    ASSERT_EQ(read_back.size(), decoded_for_write.size());
    for (size_t i = 0; i < read_back.size(); ++i) {
        EXPECT_EQ(read_back[i].command.type, decoded_for_write[i].command.type);
        EXPECT_EQ(read_back[i].command.order_id, decoded_for_write[i].command.order_id);
        EXPECT_EQ(read_back[i].command.side, decoded_for_write[i].command.side);
        EXPECT_EQ(read_back[i].command.order_type, decoded_for_write[i].command.order_type);
        EXPECT_EQ(read_back[i].command.price, decoded_for_write[i].command.price);
        EXPECT_EQ(read_back[i].command.quantity, decoded_for_write[i].command.quantity);
        EXPECT_EQ(read_back[i].command.symbol, decoded_for_write[i].command.symbol);
        EXPECT_EQ(read_back[i].timestamp, decoded_for_write[i].timestamp);
    }
}
