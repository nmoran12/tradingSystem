#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace matching_engine {

enum class EngineEventType {
    OrderAccepted,
    OrderRejected,
    OrderCancelled,
    Trade,
    BookUpdate
};

struct MatchTrade {
    uint64_t aggressive_order_id{};
    uint64_t resting_order_id{};
    int64_t price{};
    uint64_t quantity{};
};

struct EngineEvent {
    EngineEventType type{EngineEventType::OrderAccepted};
    uint64_t order_id{};
    std::optional<MatchTrade> trade;
    std::string reason;
};

}  // namespace matching_engine
