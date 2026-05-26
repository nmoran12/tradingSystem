#include "market_data/OrderCommandParser.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string_view>

namespace market_data {
namespace {

std::string trim(std::string_view value) {
    const auto start = value.find_first_not_of(" \t\r\n");
    if (start == std::string_view::npos) {
        return {};
    }
    const auto end = value.find_last_not_of(" \t\r\n");
    return std::string(value.substr(start, end - start + 1));
}

std::vector<std::string> split_csv(const std::string& line) {
    std::vector<std::string> fields;
    fields.reserve(6);
    std::stringstream stream(line);
    std::string field;
    while (std::getline(stream, field, ',')) {
        fields.push_back(trim(field));
    }
    return fields;
}

uint64_t parse_uint64(const std::string& value, const std::string& field_name) {
    try {
        return static_cast<uint64_t>(std::stoull(value));
    } catch (const std::exception&) {
        throw std::invalid_argument("invalid " + field_name + ": " + value);
    }
}

int64_t parse_int64(const std::string& value, const std::string& field_name) {
    try {
        return static_cast<int64_t>(std::stoll(value));
    } catch (const std::exception&) {
        throw std::invalid_argument("invalid " + field_name + ": " + value);
    }
}

bool is_header_row(const std::vector<std::string>& fields) {
    return fields.size() == 6 && fields[0] == "type" && fields[1] == "order_id" &&
           fields[2] == "side" && fields[3] == "order_type" && fields[4] == "price" &&
           fields[5] == "quantity";
}

matching_engine::OrderCommandType parse_command_type(const std::string& value) {
    if (value == "NEW") {
        return matching_engine::OrderCommandType::NewOrder;
    }
    if (value == "CANCEL") {
        return matching_engine::OrderCommandType::CancelOrder;
    }
    if (value == "MODIFY") {
        return matching_engine::OrderCommandType::ModifyOrder;
    }
    throw std::invalid_argument("invalid command type: " + value);
}

Side parse_side_field(const std::string& value) {
    if (value == "BUY") {
        return Side::BUY;
    }
    if (value == "SELL") {
        return Side::SELL;
    }
    if (value.empty() || value == "NONE") {
        return Side::UNKNOWN;
    }
    throw std::invalid_argument("invalid side: " + value);
}

matching_engine::OrderType parse_order_type(const std::string& value) {
    if (value == "LIMIT") {
        return matching_engine::OrderType::Limit;
    }
    if (value == "MARKET") {
        return matching_engine::OrderType::Market;
    }
    throw std::invalid_argument("invalid order type: " + value);
}

}  // namespace

std::vector<matching_engine::OrderCommand> OrderCommandParser::parse_file(const std::string& path) {
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("failed to open file: " + path);
    }
    return parse_stream(input);
}

std::vector<matching_engine::OrderCommand> OrderCommandParser::parse_stream(std::istream& input) {
    std::vector<matching_engine::OrderCommand> commands;
    std::string line;

    while (std::getline(input, line)) {
        const auto trimmed = trim(line);
        if (trimmed.empty()) {
            continue;
        }

        if (is_header_row(split_csv(trimmed))) {
            continue;
        }

        commands.push_back(parse_line(trimmed));
    }

    return commands;
}

matching_engine::OrderCommand OrderCommandParser::parse_line(const std::string& line) {
    const auto fields = split_csv(line);
    if (fields.size() != 6) {
        throw std::invalid_argument(
            "malformed CSV row: expected 6 fields, got " + std::to_string(fields.size()) +
            " in line: " + line);
    }

    if (is_header_row(fields)) {
        throw std::invalid_argument("header row is not a command");
    }

    matching_engine::OrderCommand command;
    command.type = parse_command_type(fields[0]);
    command.order_id = parse_uint64(fields[1], "order_id");
    command.side = parse_side_field(fields[2]);
    command.order_type = parse_order_type(fields[3]);
    command.price = parse_int64(fields[4], "price");
    command.quantity = parse_uint64(fields[5], "quantity");

    if (command.type == matching_engine::OrderCommandType::CancelOrder) {
        return command;
    }

    if (command.side != Side::BUY && command.side != Side::SELL) {
        throw std::invalid_argument("command requires BUY or SELL side");
    }

    return command;
}

}  // namespace market_data
