#include "protocol/BinaryCommandReader.hpp"

#include <gtest/gtest.h>

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

std::filesystem::path test_file_path(const std::string& suffix) {
    const auto* test_info = ::testing::UnitTest::GetInstance()->current_test_info();
    return std::filesystem::current_path() /
           ("orderbook_" + std::string(test_info->test_suite_name()) + "_" +
            test_info->name() + suffix);
}

class CliVisualisationExportTempFiles {
public:
    CliVisualisationExportTempFiles()
        : binary_path_(test_file_path(".obk")),
          export_path_(test_file_path(".ndjson")),
          stdout_path_(test_file_path(".stdout")),
          stderr_path_(test_file_path(".stderr")) {
        remove_files();
    }

    ~CliVisualisationExportTempFiles() {
        remove_files();
    }

    const std::filesystem::path& binary_path() const {
        return binary_path_;
    }

    const std::filesystem::path& export_path() const {
        return export_path_;
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
        std::filesystem::remove(export_path_, ignored);
        std::filesystem::remove(stdout_path_, ignored);
        std::filesystem::remove(stderr_path_, ignored);
    }

    std::filesystem::path binary_path_;
    std::filesystem::path export_path_;
    std::filesystem::path stdout_path_;
    std::filesystem::path stderr_path_;
};

int run_export_cli(const CliVisualisationExportTempFiles& files) {
    const std::string arguments = "--binary-engine " + shell_quote(files.binary_path()) +
                                  " --export-visualisation " +
                                  shell_quote(files.export_path());
    const std::string command = "./cpp-low-latency-orderbook " + arguments + " > " +
                                shell_quote(files.stdout_path()) + " 2> " +
                                shell_quote(files.stderr_path());
    return std::system(command.c_str());
}

std::vector<std::string> read_ndjson_lines(const std::filesystem::path& path) {
    std::ifstream input(path);
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(input, line)) {
        if (!line.empty()) {
            lines.push_back(line);
        }
    }
    return lines;
}

void expect_field_present(const std::string& line, const std::string& field) {
    EXPECT_NE(line.find(field), std::string::npos) << "missing field " << field;
}

void expect_export_line_shape(const std::string& line) {
    expect_field_present(line, "\"schemaVersion\":1");
    expect_field_present(line, "\"index\":");
    expect_field_present(line, "\"commandType\":");
    expect_field_present(line, "\"side\":");
    expect_field_present(line, "\"orderType\":");
    expect_field_present(line, "\"orderId\":");
    expect_field_present(line, "\"price\":");
    expect_field_present(line, "\"quantity\":");
    expect_field_present(line, "\"symbol\":");
    expect_field_present(line, "\"bestBid\":");
    expect_field_present(line, "\"bestAsk\":");
    expect_field_present(line, "\"spread\":");
    expect_field_present(line, "\"restingBidLevels\":");
    expect_field_present(line, "\"restingAskLevels\":");
    expect_field_present(line, "\"trades\":");
    expect_field_present(line, "\"totalRestingOrders\":");
    expect_field_present(line, "\"totalRestingQuantity\":");
    EXPECT_EQ(line.front(), '{');
    EXPECT_EQ(line.back(), '}');
}

bool line_has_non_empty_trades(const std::string& line) {
    const auto trades_pos = line.find("\"trades\":");
    if (trades_pos == std::string::npos) {
        return false;
    }
    const auto array_start = line.find('[', trades_pos);
    const auto array_end = line.find(']', array_start);
    if (array_start == std::string::npos || array_end == std::string::npos ||
        array_end <= array_start + 1) {
        return false;
    }
    return true;
}

}  // namespace

TEST(CliVisualisationExportTest, BinaryEngineExportCreatesNdjsonWithOneLinePerCommand) {
    CliVisualisationExportTempFiles files;
    const std::vector<DecodedOrderCommand> commands = {
        make_decoded(OrderCommandType::NewOrder, 1, Side::SELL, OrderType::Limit, 100, 10,
                     "AAPL", 1001),
        make_decoded(OrderCommandType::NewOrder, 2, Side::BUY, OrderType::Limit, 101, 10,
                     "AAPL", 1002),
        make_decoded(OrderCommandType::NewOrder, 3, Side::BUY, OrderType::Limit, 99, 5, "AAPL",
                     1003),
    };
    write_order_commands_binary(files.binary_path(), commands);

    EXPECT_EQ(run_export_cli(files), 0);
    ASSERT_TRUE(std::filesystem::exists(files.export_path()));

    const auto lines = read_ndjson_lines(files.export_path());
    ASSERT_EQ(lines.size(), commands.size());
    for (std::size_t i = 0; i < lines.size(); ++i) {
        expect_export_line_shape(lines[i]);
        expect_field_present(lines[i], "\"index\":" + std::to_string(i));
    }
}

TEST(CliVisualisationExportTest, BinaryEngineExportIncludesTradeWhenOrdersCross) {
    CliVisualisationExportTempFiles files;
    const std::vector<DecodedOrderCommand> commands = {
        make_decoded(OrderCommandType::NewOrder, 1, Side::SELL, OrderType::Limit, 100, 10,
                     "AAPL", 1001),
        make_decoded(OrderCommandType::NewOrder, 2, Side::BUY, OrderType::Limit, 101, 10,
                     "AAPL", 1002),
    };
    write_order_commands_binary(files.binary_path(), commands);

    EXPECT_EQ(run_export_cli(files), 0);

    const auto lines = read_ndjson_lines(files.export_path());
    ASSERT_EQ(lines.size(), 2u);

    EXPECT_FALSE(line_has_non_empty_trades(lines[0]));
    ASSERT_TRUE(line_has_non_empty_trades(lines[1]));
    expect_field_present(lines[1], "\"price\":100");
    expect_field_present(lines[1], "\"quantity\":10");
    expect_field_present(lines[1], "\"aggressiveOrderId\":2");
    expect_field_present(lines[1], "\"restingOrderId\":1");
}

TEST(CliVisualisationExportTest, BinaryEngineExportEmptyFileProducesNoNdjsonLines) {
    CliVisualisationExportTempFiles files;
    std::ofstream output(files.binary_path(), std::ios::binary | std::ios::trunc);
    output.close();

    EXPECT_EQ(run_export_cli(files), 0);
    ASSERT_TRUE(std::filesystem::exists(files.export_path()));

    const auto lines = read_ndjson_lines(files.export_path());
    EXPECT_TRUE(lines.empty());
}
