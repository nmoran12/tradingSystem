#pragma once

#include "market_data/MarketEvent.hpp"

#include <istream>
#include <string>
#include <vector>

namespace market_data {

class MarketDataParser {
public:
    static std::vector<MarketEvent> parse_file(const std::string& path);
    static std::vector<MarketEvent> parse_stream(std::istream& input);
    static MarketEvent parse_line(const std::string& line, bool is_header = false);
};

}  // namespace market_data
