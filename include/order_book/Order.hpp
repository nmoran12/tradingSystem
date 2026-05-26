#pragma once

#include "market_data/MarketEvent.hpp"

#include <cstdint>
#include <string>

namespace order_book {

struct Order {
    uint64_t order_id{};
    std::string symbol;
    market_data::Side side{market_data::Side::UNKNOWN};
    int64_t price{};
    uint32_t quantity{};
    uint64_t timestamp{};
};

}  // namespace order_book
