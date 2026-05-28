#include "matching_engine/MatchingEngine.hpp"

#include <algorithm>
#include <limits>
#include <utility>

namespace matching_engine {
namespace {

constexpr uint64_t kMaxOrderQuantity = std::numeric_limits<uint32_t>::max();

bool is_valid_side(market_data::Side side) {
    return side == market_data::Side::BUY || side == market_data::Side::SELL;
}

}  // namespace

MatchingEngine::MatchingEngine(std::string symbol) : symbol_(std::move(symbol)) {}

std::vector<EngineEvent> MatchingEngine::process(const OrderCommand& command) {
    switch (command.type) {
        case OrderCommandType::NewOrder:
            return process_new_order(command);
        case OrderCommandType::CancelOrder:
            return process_cancel(command);
        case OrderCommandType::ModifyOrder:
            return process_modify(command);
    }
    return reject(command.order_id, "unknown command type");
}

std::vector<EngineEvent> MatchingEngine::process_new_order(const OrderCommand& command) {
    if (!is_valid_side(command.side)) {
        return reject(command.order_id, "invalid side");
    }
    if (command.quantity == 0 || command.quantity > kMaxOrderQuantity) {
        return reject(command.order_id, "invalid quantity");
    }
    if (book_.contains_order(command.order_id)) {
        return reject(command.order_id, "duplicate order id");
    }
    if (command.order_type == OrderType::Limit && command.price <= 0) {
        return reject(command.order_id, "invalid limit price");
    }

    return execute_new_order(command.order_id, command.side, command.order_type, command.price,
                             command.quantity);
}

std::vector<EngineEvent> MatchingEngine::process_modify(const OrderCommand& command) {
    const auto existing = book_.get_order(command.order_id);
    if (!existing) {
        return reject(command.order_id, "unknown order id");
    }

    if (is_valid_side(command.side) && command.side != existing->side) {
        return reject(command.order_id, "side change not allowed");
    }

    const int64_t new_price =
        command.price > 0 ? command.price : existing->price;
    const uint64_t new_quantity =
        command.quantity > 0 ? command.quantity : existing->quantity;

    if (new_quantity == 0 || new_quantity > kMaxOrderQuantity) {
        return reject(command.order_id, "invalid quantity");
    }

    const OrderType new_type = command.order_type;
    if (new_type == OrderType::Limit && new_price <= 0) {
        return reject(command.order_id, "invalid limit price");
    }

    // Cancel-and-reinsert: modification always loses queue position.
    if (!book_.cancel_order(command.order_id)) {
        return reject(command.order_id, "modify cancel failed");
    }

    return execute_new_order(command.order_id, existing->side, new_type, new_price, new_quantity);
}

std::vector<EngineEvent> MatchingEngine::execute_new_order(uint64_t order_id,
                                                           market_data::Side side,
                                                           OrderType order_type, int64_t price,
                                                           uint64_t quantity) {
    std::vector<EngineEvent> events;
    events.reserve(4);
    events.push_back({EngineEventType::OrderAccepted, order_id, std::nullopt, {}});

    const size_t match_start_index = events.size();
    if (order_type == OrderType::Market) {
        if (side == market_data::Side::BUY) {
            match_buy_market(order_id, quantity, events);
        } else {
            match_sell_market(order_id, quantity, events);
        }
    } else if (side == market_data::Side::BUY) {
        match_buy_limit(order_id, price, quantity, events);
    } else {
        match_sell_limit(order_id, price, quantity, events);
    }

    const uint64_t filled = filled_quantity_since(events, match_start_index);
    const uint64_t remaining = quantity - filled;

    if (order_type == OrderType::Market) {
        append_market_remainder_cancel(events, order_id, remaining);
        return events;
    }

    if (remaining > 0) {
        const auto resting =
            make_resting_order(order_id, side, price, static_cast<uint32_t>(remaining));
        if (!book_.add_order(resting)) {
            return reject(order_id, "failed to rest order");
        }
        events.push_back(make_book_update(order_id));
    }

    return events;
}

std::vector<EngineEvent> MatchingEngine::process_cancel(const OrderCommand& command) {
    if (!book_.cancel_order(command.order_id)) {
        return reject(command.order_id, "unknown order id");
    }
    return {{EngineEventType::OrderCancelled, command.order_id, std::nullopt, {}}};
}

void MatchingEngine::match_buy_limit(uint64_t order_id, int64_t price, uint64_t quantity,
                                     std::vector<EngineEvent>& events) {
    uint64_t remaining = quantity;

    while (remaining > 0) {
        const auto resting = book_.peek_best_ask();
        if (!resting || price < resting->price) {
            break;
        }

        const uint64_t fill_qty = std::min(remaining, static_cast<uint64_t>(resting->quantity));
        if (!book_.reduce_order(resting->order_id, static_cast<uint32_t>(fill_qty))) {
            break;
        }

        MatchTrade trade;
        trade.aggressive_order_id = order_id;
        trade.resting_order_id = resting->order_id;
        trade.price = resting->price;
        trade.quantity = fill_qty;
        events.push_back(make_trade_event(trade));
        remaining -= fill_qty;
    }
}

void MatchingEngine::match_sell_limit(uint64_t order_id, int64_t price, uint64_t quantity,
                                      std::vector<EngineEvent>& events) {
    uint64_t remaining = quantity;

    while (remaining > 0) {
        const auto resting = book_.peek_best_bid();
        if (!resting || price > resting->price) {
            break;
        }

        const uint64_t fill_qty = std::min(remaining, static_cast<uint64_t>(resting->quantity));
        if (!book_.reduce_order(resting->order_id, static_cast<uint32_t>(fill_qty))) {
            break;
        }

        MatchTrade trade;
        trade.aggressive_order_id = order_id;
        trade.resting_order_id = resting->order_id;
        trade.price = resting->price;
        trade.quantity = fill_qty;
        events.push_back(make_trade_event(trade));
        remaining -= fill_qty;
    }
}

void MatchingEngine::match_buy_market(uint64_t order_id, uint64_t quantity,
                                      std::vector<EngineEvent>& events) {
    uint64_t remaining = quantity;

    while (remaining > 0) {
        const auto resting = book_.peek_best_ask();
        if (!resting) {
            break;
        }

        const uint64_t fill_qty = std::min(remaining, static_cast<uint64_t>(resting->quantity));
        if (!book_.reduce_order(resting->order_id, static_cast<uint32_t>(fill_qty))) {
            break;
        }

        MatchTrade trade;
        trade.aggressive_order_id = order_id;
        trade.resting_order_id = resting->order_id;
        trade.price = resting->price;
        trade.quantity = fill_qty;
        events.push_back(make_trade_event(trade));
        remaining -= fill_qty;
    }
}

void MatchingEngine::match_sell_market(uint64_t order_id, uint64_t quantity,
                                       std::vector<EngineEvent>& events) {
    uint64_t remaining = quantity;

    while (remaining > 0) {
        const auto resting = book_.peek_best_bid();
        if (!resting) {
            break;
        }

        const uint64_t fill_qty = std::min(remaining, static_cast<uint64_t>(resting->quantity));
        if (!book_.reduce_order(resting->order_id, static_cast<uint32_t>(fill_qty))) {
            break;
        }

        MatchTrade trade;
        trade.aggressive_order_id = order_id;
        trade.resting_order_id = resting->order_id;
        trade.price = resting->price;
        trade.quantity = fill_qty;
        events.push_back(make_trade_event(trade));
        remaining -= fill_qty;
    }
}

uint64_t MatchingEngine::filled_quantity_since(const std::vector<EngineEvent>& events,
                                               size_t start_index) {
    uint64_t filled = 0;
    for (size_t i = start_index; i < events.size(); ++i) {
        const auto& event = events[i];
        if (event.type == EngineEventType::Trade && event.trade) {
            filled += event.trade->quantity;
        }
    }
    return filled;
}

void MatchingEngine::append_market_remainder_cancel(std::vector<EngineEvent>& events,
                                                    uint64_t order_id, uint64_t remaining) {
    if (remaining == 0) {
        return;
    }
    events.push_back({EngineEventType::OrderCancelled, order_id, std::nullopt,
                      "unfilled market quantity"});
}

order_book::Order MatchingEngine::make_resting_order(uint64_t order_id, market_data::Side side,
                                                     int64_t price, uint32_t quantity) {
    order_book::Order order;
    order.order_id = order_id;
    order.symbol = symbol_;
    order.side = side;
    order.price = price;
    order.quantity = quantity;
    order.timestamp = next_timestamp_++;
    return order;
}

std::vector<EngineEvent> MatchingEngine::reject(uint64_t order_id, std::string reason) {
    return {{EngineEventType::OrderRejected, order_id, std::nullopt, std::move(reason)}};
}

EngineEvent MatchingEngine::make_trade_event(const MatchTrade& trade) {
    return {EngineEventType::Trade, trade.aggressive_order_id, trade, {}};
}

EngineEvent MatchingEngine::make_book_update(uint64_t order_id) {
    return {EngineEventType::BookUpdate, order_id, std::nullopt, {}};
}

}  // namespace matching_engine
