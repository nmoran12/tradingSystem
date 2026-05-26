#pragma once

#include <cstdint>
#include <string>

namespace order_book {

struct Trade {
    uint64_t buy_order_id{};
    uint64_t sell_order_id{};
    std::string symbol;
    int64_t price{};
    uint32_t quantity{};
    uint64_t timestamp{};
};

}  // namespace order_book
