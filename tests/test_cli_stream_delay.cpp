#include "protocol/BinaryCommandReader.hpp"

#include <gtest/gtest.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

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

TEST(StreamDelayCliTest, RejectsInvalidStreamDelayValue) {
    CliTempFiles files;
    const std::vector<DecodedOrderCommand> commands = {
        make_decoded(OrderCommandType::NewOrder, 1, Side::SELL, OrderType::Limit, 100, 10,
                     "AAPL", 1001),
    };
    write_order_commands_binary(files.binary_path(), commands);

    const int result = run_cli(
        "--binary-engine " + shell_quote(files.binary_path()) +
            " --stream-visualisation 127.0.0.1:1 --stream-delay-ms not-a-number",
        files);

    EXPECT_NE(result, 0);
    const auto stderr_text = read_text_file(files.stderr_path());
    EXPECT_NE(stderr_text.find("invalid --stream-delay-ms"), std::string::npos);
}

TEST(StreamDelayCliTest, RejectsStreamDelayWithoutStreamMode) {
    CliTempFiles files;
    const std::vector<DecodedOrderCommand> commands = {
        make_decoded(OrderCommandType::NewOrder, 1, Side::SELL, OrderType::Limit, 100, 10,
                     "AAPL", 1001),
    };
    write_order_commands_binary(files.binary_path(), commands);

    const int result = run_cli(
        "--binary-engine " + shell_quote(files.binary_path()) + " --stream-delay-ms 5",
        files);

    EXPECT_NE(result, 0);
    const auto stderr_text = read_text_file(files.stderr_path());
    EXPECT_NE(stderr_text.find("--stream-delay-ms requires --stream-visualisation"),
              std::string::npos);
}

TEST(StreamDelayCliTest, RejectsMissingStreamDelayValue) {
    CliTempFiles files;
    const std::vector<DecodedOrderCommand> commands = {
        make_decoded(OrderCommandType::NewOrder, 1, Side::SELL, OrderType::Limit, 100, 10,
                     "AAPL", 1001),
    };
    write_order_commands_binary(files.binary_path(), commands);

    const int result = run_cli(
        "--binary-engine " + shell_quote(files.binary_path()) +
            " --stream-visualisation 127.0.0.1:1 --stream-delay-ms",
        files);

    EXPECT_NE(result, 0);
    const auto stderr_text = read_text_file(files.stderr_path());
    EXPECT_NE(stderr_text.find("--binary-engine <order-commands.obk>"), std::string::npos);
}
