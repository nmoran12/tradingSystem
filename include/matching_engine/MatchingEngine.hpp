#pragma once

#include "matching_engine/EngineEvent.hpp"
#include "matching_engine/OrderCommand.hpp"
#include "order_book/OrderBook.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace matching_engine {

class MatchingEngine {
public:
    explicit MatchingEngine(std::string symbol = "DEFAULT");

    std::vector<EngineEvent> process(const OrderCommand& command);
    [[nodiscard]] const order_book::OrderBook& book() const { return book_; }

private:
    std::vector<EngineEvent> process_new_order(const OrderCommand& command);
    std::vector<EngineEvent> process_cancel(const OrderCommand& command);
    std::vector<EngineEvent> process_modify(const OrderCommand& command);

    std::vector<EngineEvent> execute_new_order(uint64_t order_id, market_data::Side side,
                                               OrderType order_type, int64_t price,
                                               uint64_t quantity);

    void match_buy_limit(uint64_t order_id, int64_t price, uint64_t quantity,
                         std::vector<EngineEvent>& events);
    void match_sell_limit(uint64_t order_id, int64_t price, uint64_t quantity,
                          std::vector<EngineEvent>& events);
    void match_buy_market(uint64_t order_id, uint64_t quantity,
                          std::vector<EngineEvent>& events);
    void match_sell_market(uint64_t order_id, uint64_t quantity,
                           std::vector<EngineEvent>& events);

    static uint64_t filled_quantity_since(const std::vector<EngineEvent>& events,
                                          size_t start_index);
    static void append_market_remainder_cancel(std::vector<EngineEvent>& events, uint64_t order_id,
                                               uint64_t remaining);

    static std::vector<EngineEvent> reject(uint64_t order_id, std::string reason);
    static EngineEvent make_trade_event(const MatchTrade& trade);
    static EngineEvent make_book_update(uint64_t order_id);

    order_book::Order make_resting_order(uint64_t order_id, market_data::Side side, int64_t price,
                                         uint32_t quantity);

    order_book::OrderBook book_;
    std::string symbol_;
    uint64_t next_timestamp_{1};
};

}  // namespace matching_engine
