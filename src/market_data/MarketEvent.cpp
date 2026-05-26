#include "market_data/MarketEvent.hpp"

#include <stdexcept>

namespace market_data {

const char* to_string(EventType type) {
    switch (type) {
        case EventType::ADD:
            return "ADD";
        case EventType::CANCEL:
            return "CANCEL";
        case EventType::EXECUTE:
            return "EXECUTE";
    }
    return "UNKNOWN";
}

const char* to_string(Side side) {
    switch (side) {
        case Side::BUY:
            return "BUY";
        case Side::SELL:
            return "SELL";
        case Side::UNKNOWN:
            return "UNKNOWN";
    }
    return "UNKNOWN";
}

EventType parse_event_type(const std::string& value) {
    if (value == "ADD") {
        return EventType::ADD;
    }
    if (value == "CANCEL") {
        return EventType::CANCEL;
    }
    if (value == "EXECUTE") {
        return EventType::EXECUTE;
    }
    throw std::invalid_argument("invalid event type: " + value);
}

Side parse_side(const std::string& value) {
    if (value == "BUY") {
        return Side::BUY;
    }
    if (value == "SELL") {
        return Side::SELL;
    }
    if (value == "UNKNOWN") {
        return Side::UNKNOWN;
    }
    throw std::invalid_argument("invalid side: " + value);
}

}  // namespace market_data
