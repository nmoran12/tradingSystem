#pragma once

#include <cstdint>
#include <string>

namespace market_data {

enum class EventType { ADD, CANCEL, EXECUTE };

enum class Side { BUY, SELL, UNKNOWN };

struct MarketEvent {
    uint64_t timestamp{};
    EventType type{EventType::ADD};
    std::string symbol;
    Side side{Side::UNKNOWN};
    uint64_t order_id{};
    int64_t price{};
    uint32_t quantity{};
};

const char* to_string(EventType type);
const char* to_string(Side side);

EventType parse_event_type(const std::string& value);
Side parse_side(const std::string& value);

}  // namespace market_data
