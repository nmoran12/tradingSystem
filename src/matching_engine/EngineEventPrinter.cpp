#include "matching_engine/EngineEventPrinter.hpp"

#include <iostream>

namespace matching_engine {

const char* to_string(EngineEventType type) {
    switch (type) {
        case EngineEventType::OrderAccepted:
            return "OrderAccepted";
        case EngineEventType::OrderRejected:
            return "OrderRejected";
        case EngineEventType::OrderCancelled:
            return "OrderCancelled";
        case EngineEventType::Trade:
            return "Trade";
        case EngineEventType::BookUpdate:
            return "BookUpdate";
    }
    return "Unknown";
}

void print_engine_event(std::ostream& out, const EngineEvent& event) {
    out << to_string(event.type) << " order_id=" << event.order_id;
    if (event.type == EngineEventType::Trade && event.trade) {
        out << " aggressive=" << event.trade->aggressive_order_id
            << " resting=" << event.trade->resting_order_id << " price=" << event.trade->price
            << " qty=" << event.trade->quantity;
    }
    if (!event.reason.empty()) {
        out << " reason=\"" << event.reason << '"';
    }
    out << '\n';
}

void print_engine_events(std::ostream& out, const std::vector<EngineEvent>& events) {
    for (const auto& event : events) {
        print_engine_event(out, event);
    }
}

}  // namespace matching_engine
