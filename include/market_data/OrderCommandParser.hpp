#pragma once

#include "matching_engine/OrderCommand.hpp"

#include <istream>
#include <string>
#include <vector>

namespace market_data {

class OrderCommandParser {
public:
    static std::vector<matching_engine::OrderCommand> parse_file(const std::string& path);
    static std::vector<matching_engine::OrderCommand> parse_stream(std::istream& input);
    static matching_engine::OrderCommand parse_line(const std::string& line);
};

}  // namespace market_data
