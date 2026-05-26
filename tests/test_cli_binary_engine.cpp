#include "protocol/BinaryCommandReader.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
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

std::string shell_quote(const std::filesystem::path& path) {
    std::string quoted = "'";
    for (const char ch : path.string()) {
        if (ch == '\'') {
            quoted += "'\\''";
        } else {
            quoted += ch;
        }
    }
    quoted += "'";
    return quoted;
}

std::string read_text_file(const std::filesystem::path& path) {
    std::ifstream input(path);
    std::ostringstream output;
    output << input.rdbuf();
    return output.str();
}

std::filesystem::path test_file_path(const std::string& suffix) {
    const auto* test_info = ::testing::UnitTest::GetInstance()->current_test_info();
    return std::filesystem::current_path() /
           ("orderbook_" + std::string(test_info->test_suite_name()) + "_" +
            test_info->name() + suffix);
}

class CliTempFiles {
public:
    CliTempFiles()
        : binary_path_(test_file_path(".obk")),
          stdout_path_(test_file_path(".stdout")),
          stderr_path_(test_file_path(".stderr")) {
        remove_files();
    }

    ~CliTempFiles() {
        remove_files();
    }

    const std::filesystem::path& binary_path() const {
        return binary_path_;
    }

    const std::filesystem::path& stdout_path() const {
        return stdout_path_;
    }

    const std::filesystem::path& stderr_path() const {
        return stderr_path_;
    }

private:
    void remove_files() {
        std::error_code ignored;
        std::filesystem::remove(binary_path_, ignored);
        std::filesystem::remove(stdout_path_, ignored);
        std::filesystem::remove(stderr_path_, ignored);
    }

    std::filesystem::path binary_path_;
    std::filesystem::path stdout_path_;
    std::filesystem::path stderr_path_;
};

int run_cli(const std::string& arguments, const CliTempFiles& files) {
    const std::string command = "./cpp-low-latency-orderbook " + arguments + " > " +
                                shell_quote(files.stdout_path()) + " 2> " +
                                shell_quote(files.stderr_path());
    return std::system(command.c_str());
}

}  // namespace

TEST(BinaryEngineCliTest, BinaryEngineAcceptsValidCommandFileAndPrintsExpectedTrade) {
    CliTempFiles files;
    const std::vector<DecodedOrderCommand> commands = {
        make_decoded(OrderCommandType::NewOrder, 1, Side::SELL, OrderType::Limit, 100, 10,
                     "AAPL", 1001),
        make_decoded(OrderCommandType::NewOrder, 2, Side::BUY, OrderType::Limit, 101, 10,
                     "AAPL", 1002),
    };
    write_order_commands_binary(files.binary_path(), commands);

    const int result =
        run_cli("--binary-engine " + shell_quote(files.binary_path()), files);

    EXPECT_EQ(result, 0);
    const auto stdout_text = read_text_file(files.stdout_path());
    EXPECT_NE(stdout_text.find("Trade order_id=2 aggressive=2 resting=1 price=100 qty=10"),
              std::string::npos);
    EXPECT_NE(stdout_text.find("Engine mode: processed 2 commands, 1 trades"),
              std::string::npos);
}

TEST(BinaryEngineCliTest, BinaryEngineRejectsTruncatedBinaryFile) {
    CliTempFiles files;
    std::ofstream output(files.binary_path(), std::ios::binary | std::ios::trunc);
    const std::array<char, 3> bytes = {'O', 'B', 'K'};
    output.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    output.close();

    const int result =
        run_cli("--binary-engine " + shell_quote(files.binary_path()), files);

    EXPECT_NE(result, 0);
    const auto stderr_text = read_text_file(files.stderr_path());
    EXPECT_NE(stderr_text.find("invalid binary command file size"), std::string::npos);
}

TEST(BinaryEngineCliTest, BinaryEngineHandlesEmptyFileAsZeroCommands) {
    CliTempFiles files;
    std::ofstream output(files.binary_path(), std::ios::binary | std::ios::trunc);
    output.close();

    const int result =
        run_cli("--binary-engine " + shell_quote(files.binary_path()), files);

    EXPECT_EQ(result, 0);
    const auto stdout_text = read_text_file(files.stdout_path());
    EXPECT_NE(stdout_text.find("Engine mode: processed 0 commands, 0 trades"),
              std::string::npos);
}

TEST(BinaryEngineCliTest, MissingBinaryEnginePathReturnsUsageError) {
    CliTempFiles files;

    const int result = run_cli("--binary-engine", files);

    EXPECT_NE(result, 0);
    const auto stderr_text = read_text_file(files.stderr_path());
    EXPECT_NE(stderr_text.find("--binary-engine <order-commands.obk>"), std::string::npos);
}

TEST(BinaryEngineCliTest, ExistingCsvEnginePathStillWorks) {
    CliTempFiles files;
    const auto csv_path = std::filesystem::current_path().parent_path() / "data" /
                          "sample_commands.csv";

    const int result = run_cli("--engine " + shell_quote(csv_path), files);

    EXPECT_EQ(result, 0);
    const auto stdout_text = read_text_file(files.stdout_path());
    EXPECT_NE(stdout_text.find("Engine mode: processed 4 commands"), std::string::npos);
}
