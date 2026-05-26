#include "protocol/BinaryCommandReader.hpp"

#include <gtest/gtest.h>

#include <array>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace market_data;
using namespace matching_engine;
using namespace protocol;

namespace {

OrderCommand make_command(OrderCommandType type, uint64_t order_id, Side side,
                          OrderType order_type, int64_t price, uint64_t quantity,
                          const std::string& symbol) {
    OrderCommand command;
    command.type = type;
    command.order_id = order_id;
    command.side = side;
    command.order_type = order_type;
    command.price = price;
    command.quantity = quantity;
    command.symbol = symbol;
    return command;
}

DecodedOrderCommand make_decoded(OrderCommandType type, uint64_t order_id, Side side,
                                 OrderType order_type, int64_t price, uint64_t quantity,
                                 const std::string& symbol, uint64_t timestamp) {
    DecodedOrderCommand decoded;
    decoded.command = make_command(type, order_id, side, order_type, price, quantity, symbol);
    decoded.timestamp = timestamp;
    return decoded;
}

void expect_commands_equal(const OrderCommand& lhs, const OrderCommand& rhs) {
    EXPECT_EQ(lhs.type, rhs.type);
    EXPECT_EQ(lhs.order_id, rhs.order_id);
    EXPECT_EQ(lhs.side, rhs.side);
    EXPECT_EQ(lhs.order_type, rhs.order_type);
    EXPECT_EQ(lhs.price, rhs.price);
    EXPECT_EQ(lhs.quantity, rhs.quantity);
    EXPECT_EQ(lhs.symbol, rhs.symbol);
}

void expect_decoded_equal(const DecodedOrderCommand& lhs, const DecodedOrderCommand& rhs) {
    expect_commands_equal(lhs.command, rhs.command);
    EXPECT_EQ(lhs.timestamp, rhs.timestamp);
}

std::filesystem::path temp_file_path() {
    const auto* test_info = ::testing::UnitTest::GetInstance()->current_test_info();
    return std::filesystem::temp_directory_path() /
           ("orderbook_" + std::string(test_info->test_suite_name()) + "_" +
            test_info->name() + ".bin");
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

TEST(BinaryCommandReaderTest, WritesAndReadsOneCommand) {
    ScopedTempFile temp_file;
    const std::vector<DecodedOrderCommand> commands = {
        make_decoded(OrderCommandType::NewOrder, 1, Side::BUY, OrderType::Limit, 10050, 10,
                     "AAPL", 1001),
    };

    write_order_commands_binary(temp_file.path(), commands);
    const auto read_back = read_order_commands_binary(temp_file.path());

    ASSERT_EQ(read_back.size(), commands.size());
    expect_decoded_equal(read_back[0], commands[0]);
}

TEST(BinaryCommandReaderTest, WritesAndReadsMultipleCommands) {
    ScopedTempFile temp_file;
    const std::vector<DecodedOrderCommand> commands = {
        make_decoded(OrderCommandType::NewOrder, 1, Side::BUY, OrderType::Limit, 10050, 10,
                     "AAPL", 1001),
        make_decoded(OrderCommandType::ModifyOrder, 1, Side::BUY, OrderType::Limit, 10075, 8,
                     "AAPL", 1002),
        make_decoded(OrderCommandType::CancelOrder, 1, Side::UNKNOWN, OrderType::Limit, 0, 0,
                     "AAPL", 1003),
    };

    write_order_commands_binary(temp_file.path(), commands);
    const auto read_back = read_order_commands_binary(temp_file.path());

    ASSERT_EQ(read_back.size(), commands.size());
    for (std::size_t i = 0; i < commands.size(); ++i) {
        expect_decoded_equal(read_back[i], commands[i]);
    }
}

TEST(BinaryCommandReaderTest, PreservesTimestampThroughFileRoundTrip) {
    ScopedTempFile temp_file;
    const std::vector<DecodedOrderCommand> commands = {
        make_decoded(OrderCommandType::NewOrder, 42, Side::SELL, OrderType::Market, 0, 99, "MSFT",
                     0x0102030405060708ULL),
    };

    write_order_commands_binary(temp_file.path(), commands);
    const auto read_back = read_order_commands_binary(temp_file.path());

    ASSERT_EQ(read_back.size(), 1u);
    EXPECT_EQ(read_back[0].timestamp, commands[0].timestamp);
}

TEST(BinaryCommandReaderTest, WriterProducesExpectedFileSize) {
    ScopedTempFile temp_file;
    const std::vector<DecodedOrderCommand> commands = {
        make_decoded(OrderCommandType::NewOrder, 1, Side::BUY, OrderType::Limit, 100, 1, "A",
                     1),
        make_decoded(OrderCommandType::NewOrder, 2, Side::SELL, OrderType::Limit, 101, 2, "B",
                     2),
    };

    write_order_commands_binary(temp_file.path(), commands);

    EXPECT_EQ(std::filesystem::file_size(temp_file.path()), commands.size() * kMessageSize);
}

TEST(BinaryCommandReaderTest, RejectsFileSizeThatIsNotMultipleOfMessageSize) {
    ScopedTempFile temp_file;
    std::ofstream output(temp_file.path(), std::ios::binary | std::ios::trunc);
    const std::array<char, 3> bytes = {'O', 'B', 'K'};
    output.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    output.close();

    EXPECT_THROW(read_order_commands_binary(temp_file.path()), std::invalid_argument);
}

TEST(BinaryCommandReaderTest, RejectsInvalidMessageContents) {
    ScopedTempFile temp_file;
    const std::vector<DecodedOrderCommand> commands = {
        make_decoded(OrderCommandType::NewOrder, 1, Side::BUY, OrderType::Limit, 100, 1, "A",
                     1),
    };
    write_order_commands_binary(temp_file.path(), commands);

    std::fstream file(temp_file.path(), std::ios::binary | std::ios::in | std::ios::out);
    file.put('\0');
    file.close();

    EXPECT_THROW(read_order_commands_binary(temp_file.path()), std::invalid_argument);
}

TEST(BinaryCommandReaderTest, ReadingEmptyFileReturnsEmptyVector) {
    ScopedTempFile temp_file;
    std::ofstream output(temp_file.path(), std::ios::binary | std::ios::trunc);
    output.close();

    const auto read_back = read_order_commands_binary(temp_file.path());

    EXPECT_TRUE(read_back.empty());
}

TEST(BinaryCommandReaderTest, StreamsOneCommand) {
    ScopedTempFile temp_file;
    const std::vector<DecodedOrderCommand> commands = {
        make_decoded(OrderCommandType::NewOrder, 1, Side::BUY, OrderType::Limit, 10050, 10,
                     "AAPL", 1001),
    };
    write_order_commands_binary(temp_file.path(), commands);

    std::vector<DecodedOrderCommand> streamed;
    const auto count = stream_order_commands_binary(temp_file.path(), [&](const auto& decoded) {
        streamed.push_back(decoded);
    });

    EXPECT_EQ(count, 1u);
    ASSERT_EQ(streamed.size(), commands.size());
    expect_decoded_equal(streamed[0], commands[0]);
}

TEST(BinaryCommandReaderTest, StreamsMultipleCommandsInOrder) {
    ScopedTempFile temp_file;
    const std::vector<DecodedOrderCommand> commands = {
        make_decoded(OrderCommandType::NewOrder, 1, Side::BUY, OrderType::Limit, 10050, 10,
                     "AAPL", 1001),
        make_decoded(OrderCommandType::NewOrder, 2, Side::SELL, OrderType::Limit, 10060, 20,
                     "MSFT", 1002),
        make_decoded(OrderCommandType::CancelOrder, 1, Side::UNKNOWN, OrderType::Limit, 0, 0,
                     "AAPL", 1003),
    };
    write_order_commands_binary(temp_file.path(), commands);

    std::vector<DecodedOrderCommand> streamed;
    const auto count = stream_order_commands_binary(temp_file.path(), [&](const auto& decoded) {
        streamed.push_back(decoded);
    });

    EXPECT_EQ(count, commands.size());
    ASSERT_EQ(streamed.size(), commands.size());
    for (std::size_t i = 0; i < commands.size(); ++i) {
        expect_decoded_equal(streamed[i], commands[i]);
    }
}

TEST(BinaryCommandReaderTest, StreamingPreservesTimestamp) {
    ScopedTempFile temp_file;
    const std::vector<DecodedOrderCommand> commands = {
        make_decoded(OrderCommandType::ModifyOrder, 42, Side::SELL, OrderType::Limit, 10100, 99,
                     "NVDA", 0x0102030405060708ULL),
    };
    write_order_commands_binary(temp_file.path(), commands);

    uint64_t timestamp = 0;
    const auto count = stream_order_commands_binary(temp_file.path(), [&](const auto& decoded) {
        timestamp = decoded.timestamp;
    });

    EXPECT_EQ(count, 1u);
    EXPECT_EQ(timestamp, commands[0].timestamp);
}

TEST(BinaryCommandReaderTest, StreamingEmptyFileReturnsZero) {
    ScopedTempFile temp_file;
    std::ofstream output(temp_file.path(), std::ios::binary | std::ios::trunc);
    output.close();

    std::size_t callback_count = 0;
    const auto count = stream_order_commands_binary(temp_file.path(), [&](const auto&) {
        ++callback_count;
    });

    EXPECT_EQ(count, 0u);
    EXPECT_EQ(callback_count, 0u);
}

TEST(BinaryCommandReaderTest, StreamingRejectsFileSizeThatIsNotMultipleOfMessageSize) {
    ScopedTempFile temp_file;
    std::ofstream output(temp_file.path(), std::ios::binary | std::ios::trunc);
    const std::array<char, 3> bytes = {'O', 'B', 'K'};
    output.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    output.close();

    EXPECT_THROW(stream_order_commands_binary(temp_file.path(), [](const auto&) {}),
                 std::invalid_argument);
}

TEST(BinaryCommandReaderTest, StreamingRejectsInvalidMessageContents) {
    ScopedTempFile temp_file;
    const std::vector<DecodedOrderCommand> commands = {
        make_decoded(OrderCommandType::NewOrder, 1, Side::BUY, OrderType::Limit, 100, 1, "A",
                     1),
    };
    write_order_commands_binary(temp_file.path(), commands);

    std::fstream file(temp_file.path(), std::ios::binary | std::ios::in | std::ios::out);
    file.put('\0');
    file.close();

    EXPECT_THROW(stream_order_commands_binary(temp_file.path(), [](const auto&) {}),
                 std::invalid_argument);
}

TEST(BinaryCommandReaderTest, StreamingCallbackRunsOncePerValidCommandBeforeInvalidMessage) {
    ScopedTempFile temp_file;
    const std::vector<DecodedOrderCommand> commands = {
        make_decoded(OrderCommandType::NewOrder, 1, Side::BUY, OrderType::Limit, 100, 1, "A",
                     1),
        make_decoded(OrderCommandType::NewOrder, 2, Side::SELL, OrderType::Limit, 101, 2, "B",
                     2),
    };
    write_order_commands_binary(temp_file.path(), commands);

    std::fstream file(temp_file.path(), std::ios::binary | std::ios::in | std::ios::out);
    file.seekp(static_cast<std::streamoff>(kMessageSize));
    file.put('\0');
    file.close();

    std::size_t callback_count = 0;
    EXPECT_THROW(stream_order_commands_binary(temp_file.path(), [&](const auto&) {
                     ++callback_count;
                 }),
                 std::invalid_argument);
    EXPECT_EQ(callback_count, 1u);
}

TEST(BinaryCommandReaderTest, BufferedReaderStillWorksAfterStreamingReaderAdded) {
    ScopedTempFile temp_file;
    const std::vector<DecodedOrderCommand> commands = {
        make_decoded(OrderCommandType::NewOrder, 10, Side::BUY, OrderType::Limit, 100, 5,
                     "AAPL", 10),
    };
    write_order_commands_binary(temp_file.path(), commands);

    const auto read_back = read_order_commands_binary(temp_file.path());

    ASSERT_EQ(read_back.size(), commands.size());
    expect_decoded_equal(read_back[0], commands[0]);
}
