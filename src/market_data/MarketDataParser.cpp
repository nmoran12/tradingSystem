#include "market_data/MarketDataParser.hpp"

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
    fields.reserve(7);
    std::stringstream stream(line);
    std::string field;
    while (std::getline(stream, field, ',')) {
        fields.push_back(trim(field));
    }
    return fields;
}

uint64_t parse_uint64(const std::string& value, const std::string& field_name) {
    try {
        const auto parsed = std::stoull(value);
        return static_cast<uint64_t>(parsed);
    } catch (const std::exception&) {
        throw std::invalid_argument("invalid " + field_name + ": " + value);
    }
}

int64_t parse_int64(const std::string& value, const std::string& field_name) {
    try {
        const auto parsed = std::stoll(value);
        return static_cast<int64_t>(parsed);
    } catch (const std::exception&) {
        throw std::invalid_argument("invalid " + field_name + ": " + value);
    }
}

uint32_t parse_uint32(const std::string& value, const std::string& field_name) {
    try {
        const auto parsed = std::stoul(value);
        return static_cast<uint32_t>(parsed);
    } catch (const std::exception&) {
        throw std::invalid_argument("invalid " + field_name + ": " + value);
    }
}

bool is_header_row(const std::vector<std::string>& fields) {
    return fields.size() == 7 && fields[0] == "timestamp" && fields[1] == "type" &&
           fields[2] == "symbol" && fields[3] == "side" && fields[4] == "order_id" &&
           fields[5] == "price" && fields[6] == "quantity";
}

}  // namespace

std::vector<MarketEvent> MarketDataParser::parse_file(const std::string& path) {
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("failed to open file: " + path);
    }
    return parse_stream(input);
}

std::vector<MarketEvent> MarketDataParser::parse_stream(std::istream& input) {
    std::vector<MarketEvent> events;
    std::string line;
    bool header_skipped = false;

    while (std::getline(input, line)) {
        const auto trimmed = trim(line);
        if (trimmed.empty()) {
            continue;
        }

        if (!header_skipped && is_header_row(split_csv(trimmed))) {
            header_skipped = true;
            continue;
        }

        events.push_back(parse_line(trimmed, false));
    }

    return events;
}

MarketEvent MarketDataParser::parse_line(const std::string& line, bool is_header) {
    if (is_header) {
        throw std::invalid_argument("header row is not a market event");
    }

    const auto fields = split_csv(line);
    if (fields.size() != 7) {
        throw std::invalid_argument(
            "malformed CSV row: expected 7 fields, got " + std::to_string(fields.size()) +
            " in line: " + line);
    }

    if (is_header_row(fields)) {
        throw std::invalid_argument("header row is not a market event");
    }

    MarketEvent event;
    event.timestamp = parse_uint64(fields[0], "timestamp");
    event.type = parse_event_type(fields[1]);
    event.symbol = fields[2];
    event.side = parse_side(fields[3]);
    event.order_id = parse_uint64(fields[4], "order_id");
    event.price = parse_int64(fields[5], "price");
    event.quantity = parse_uint32(fields[6], "quantity");

    if (event.symbol.empty()) {
        throw std::invalid_argument("symbol must not be empty");
    }

    return event;
}

}  // namespace market_data
