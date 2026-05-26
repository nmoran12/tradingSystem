#pragma once

#include "market_data/MarketEvent.hpp"

#include <cstdint>
#include <string>

namespace matching_engine {

enum class OrderCommandType { NewOrder, CancelOrder, ModifyOrder };

enum class OrderType { Limit, Market };

struct OrderCommand {
    OrderCommandType type{OrderCommandType::NewOrder};
    uint64_t order_id{};
    market_data::Side side{market_data::Side::UNKNOWN};
    OrderType order_type{OrderType::Limit};
    int64_t price{};
    uint64_t quantity{};
    std::string symbol{"DEFAULT"};
};

}  // namespace matching_engine
