#include "benchmarks/WorkloadGenerator.hpp"
#include "market_data/OrderCommandParser.hpp"
#include "matching_engine/EngineEvent.hpp"
#include "matching_engine/MatchingEngine.hpp"
#include "protocol/BinaryCommandReader.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

using namespace benchmarks;
using namespace market_data;
using namespace matching_engine;
using namespace protocol;

namespace {

std::string command_type_token(OrderCommandType type) {
    switch (type) {
        case OrderCommandType::NewOrder:
            return "NEW";
        case OrderCommandType::CancelOrder:
            return "CANCEL";
        case OrderCommandType::ModifyOrder:
            return "MODIFY";
    }
    return "NEW";
}

std::string side_token(Side side) {
    switch (side) {
        case Side::BUY:
            return "BUY";
        case Side::SELL:
            return "SELL";
        case Side::UNKNOWN:
            return "BUY";
    }
    return "BUY";
}

std::string order_type_token(OrderType order_type) {
    switch (order_type) {
        case OrderType::Limit:
            return "LIMIT";
        case OrderType::Market:
            return "MARKET";
    }
    return "LIMIT";
}

void write_commands_csv(const std::filesystem::path& path,
                        const std::vector<OrderCommand>& commands) {
    std::ofstream output(path);
    if (!output) {
        throw std::runtime_error("unable to open csv path for writing: " + path.string());
    }
    output << "type,order_id,side,order_type,price,quantity\n";
    for (const auto& command : commands) {
        output << command_type_token(command.type) << ',' << command.order_id << ','
               << side_token(command.side) << ',' << order_type_token(command.order_type) << ','
               << command.price << ',' << command.quantity << '\n';
    }
}

std::vector<DecodedOrderCommand> attach_timestamps(const std::vector<OrderCommand>& commands) {
    std::vector<DecodedOrderCommand> decoded;
    decoded.reserve(commands.size());
    uint64_t timestamp = 0;
    for (const auto& command : commands) {
        decoded.push_back({command, timestamp++});
    }
    return decoded;
}

uint64_t count_trades(const std::vector<EngineEvent>& events) {
    uint64_t trades = 0;
    for (const auto& event : events) {
        if (event.type == EngineEventType::Trade && event.trade) {
            ++trades;
        }
    }
    return trades;
}

void process_commands(MatchingEngine& engine, const std::vector<OrderCommand>& commands,
                      uint64_t& trade_count) {
    std::vector<EngineEvent> scratch;
    scratch.reserve(16);
    for (const auto& command : commands) {
        engine.process_into(command, scratch);
        trade_count += count_trades(scratch);
    }
}

void expect_books_equivalent(const MatchingEngine& lhs, const MatchingEngine& rhs) {
    const auto& left_book = lhs.book();
    const auto& right_book = rhs.book();

    EXPECT_EQ(left_book.active_order_count(), right_book.active_order_count());
    EXPECT_EQ(left_book.best_bid(), right_book.best_bid());
    EXPECT_EQ(left_book.best_ask(), right_book.best_ask());
    EXPECT_EQ(left_book.total_resting_quantity(), right_book.total_resting_quantity());
    EXPECT_EQ(left_book.validate_invariants(), right_book.validate_invariants());
}

void expect_engines_equivalent(const MatchingEngine& lhs, uint64_t lhs_trades,
                               const MatchingEngine& rhs, uint64_t rhs_trades) {
    expect_books_equivalent(lhs, rhs);
    EXPECT_EQ(lhs_trades, rhs_trades);
    EXPECT_FALSE(lhs.book().validate_invariants().has_value());
    EXPECT_FALSE(rhs.book().validate_invariants().has_value());
}

std::filesystem::path temp_path(const std::string& suffix) {
    const auto* test_info = ::testing::UnitTest::GetInstance()->current_test_info();
    return std::filesystem::temp_directory_path() /
           ("orderbook_equiv_" + std::string(test_info->test_suite_name()) + "_" +
            test_info->name() + suffix);
}

class TempPathCleanup {
public:
    explicit TempPathCleanup(std::filesystem::path path) : path_(std::move(path)) {}
    ~TempPathCleanup() { std::error_code error; std::filesystem::remove(path_, error); }

private:
    std::filesystem::path path_;
};

}  // namespace

TEST(CsvBinaryEngineEquivalenceTest, WorkloadSeed42MatchesCsvAndBinaryPaths) {
    WorkloadConfig config;
    config.command_count = 150;
    config.random_seed = 42;
    const auto commands = WorkloadGenerator(config).generate();
    ASSERT_EQ(commands.size(), 150u);

    MatchingEngine reference_engine;
    uint64_t reference_trades = 0;
    process_commands(reference_engine, commands, reference_trades);

    const auto csv_path = temp_path(".csv");
    const auto binary_path = temp_path(".obk");
    TempPathCleanup csv_cleanup(csv_path);
    TempPathCleanup binary_cleanup(binary_path);

    write_commands_csv(csv_path, commands);
    write_order_commands_binary(binary_path, attach_timestamps(commands));

    const auto csv_commands = OrderCommandParser::parse_file(csv_path.string());
    ASSERT_EQ(csv_commands.size(), commands.size());

    MatchingEngine csv_engine;
    uint64_t csv_trades = 0;
    process_commands(csv_engine, csv_commands, csv_trades);

    const auto decoded = read_order_commands_binary(binary_path);
    ASSERT_EQ(decoded.size(), commands.size());

    MatchingEngine binary_engine;
    uint64_t binary_trades = 0;
    std::vector<OrderCommand> binary_commands;
    binary_commands.reserve(decoded.size());
    for (const auto& entry : decoded) {
        binary_commands.push_back(entry.command);
    }
    process_commands(binary_engine, binary_commands, binary_trades);

    expect_engines_equivalent(reference_engine, reference_trades, csv_engine, csv_trades);
    expect_engines_equivalent(reference_engine, reference_trades, binary_engine, binary_trades);
}

TEST(CsvBinaryEngineEquivalenceTest, SampleCommandsCsvMatchesBinaryRoundTrip) {
    const auto sample_csv =
        std::filesystem::current_path().parent_path() / "data" / "sample_commands.csv";
    ASSERT_TRUE(std::filesystem::exists(sample_csv));

    const auto commands = OrderCommandParser::parse_file(sample_csv.string());
    ASSERT_FALSE(commands.empty());

    MatchingEngine reference_engine;
    uint64_t reference_trades = 0;
    process_commands(reference_engine, commands, reference_trades);

    const auto binary_path = temp_path(".obk");
    TempPathCleanup binary_cleanup(binary_path);
    write_order_commands_binary(binary_path, attach_timestamps(commands));

    const auto decoded = read_order_commands_binary(binary_path);
    ASSERT_EQ(decoded.size(), commands.size());

    MatchingEngine binary_engine;
    uint64_t binary_trades = 0;
    std::vector<OrderCommand> binary_commands;
    binary_commands.reserve(decoded.size());
    for (const auto& entry : decoded) {
        binary_commands.push_back(entry.command);
    }
    process_commands(binary_engine, binary_commands, binary_trades);

    expect_engines_equivalent(reference_engine, reference_trades, binary_engine, binary_trades);
}
