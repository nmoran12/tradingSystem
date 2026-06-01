#include "viz/ReplayVisualisationWriter.hpp"

#include "market_data/MarketEvent.hpp"

#include <sstream>

namespace viz {
namespace {

std::string to_string(matching_engine::OrderCommandType type) {
    switch (type) {
        case matching_engine::OrderCommandType::NewOrder:
            return "new";
        case matching_engine::OrderCommandType::CancelOrder:
            return "cancel";
        case matching_engine::OrderCommandType::ModifyOrder:
            return "modify";
    }
    return "unknown";
}

std::string to_side_string(market_data::Side side) {
    switch (side) {
        case market_data::Side::BUY:
            return "buy";
        case market_data::Side::SELL:
            return "sell";
        case market_data::Side::UNKNOWN:
        default:
            return "unknown";
    }
}

std::string to_order_type_string(matching_engine::OrderType type) {
    switch (type) {
        case matching_engine::OrderType::Limit:
            return "limit";
        case matching_engine::OrderType::Market:
            return "market";
    }
    return "unknown";
}

void write_record_body(std::ostream& out, std::size_t index,
                       const matching_engine::OrderCommand& command,
                       const std::vector<matching_engine::EngineEvent>& events,
                       const order_book::OrderBook& book) {
    const auto best_bid = book.best_bid();
    const auto best_ask = book.best_ask();
    const auto spread = book.spread();

    uint32_t best_bid_quantity = 0;
    if (best_bid) {
        best_bid_quantity =
            book.total_quantity_at_price(market_data::Side::BUY, *best_bid);
    }

    uint32_t best_ask_quantity = 0;
    if (best_ask) {
        best_ask_quantity =
            book.total_quantity_at_price(market_data::Side::SELL, *best_ask);
    }

    const auto active_orders = book.active_order_count();
    const auto resting_quantity = book.total_resting_quantity();

    out << '{';
    out << "\"schemaVersion\":1";
    out << ",\"index\":" << index;
    out << ",\"commandType\":\"" << to_string(command.type) << '"';
    out << ",\"side\":\"" << to_side_string(command.side) << '"';
    out << ",\"orderType\":\"" << to_order_type_string(command.order_type) << '"';
    out << ",\"orderId\":" << command.order_id;
    out << ",\"price\":" << command.price;
    out << ",\"quantity\":" << command.quantity;
    out << ",\"symbol\":\"" << command.symbol << '"';

    if (best_bid) {
        out << ",\"bestBid\":" << *best_bid;
    } else {
        out << ",\"bestBid\":null";
    }
    if (best_ask) {
        out << ",\"bestAsk\":" << *best_ask;
    } else {
        out << ",\"bestAsk\":null";
    }
    if (spread) {
        out << ",\"spread\":" << *spread;
    } else {
        out << ",\"spread\":null";
    }

    out << ",\"restingBidLevels\":[";
    if (best_bid && best_bid_quantity > 0) {
        out << "{\"price\":" << *best_bid << ",\"quantity\":" << best_bid_quantity
            << "}";
    }
    out << ']';

    out << ",\"restingAskLevels\":[";
    if (best_ask && best_ask_quantity > 0) {
        out << "{\"price\":" << *best_ask << ",\"quantity\":" << best_ask_quantity
            << "}";
    }
    out << ']';

    out << ",\"trades\":[";
    bool first_trade = true;
    for (const auto& event : events) {
        if (event.type != matching_engine::EngineEventType::Trade || !event.trade) {
            continue;
        }
        if (!first_trade) {
            out << ',';
        }
        first_trade = false;
        const auto& trade = *event.trade;
        out << '{';
        out << "\"price\":" << trade.price;
        out << ",\"quantity\":" << trade.quantity;
        out << ",\"aggressiveOrderId\":" << trade.aggressive_order_id;
        out << ",\"restingOrderId\":" << trade.resting_order_id;
        out << '}';
    }
    out << ']';

    out << ",\"totalRestingOrders\":" << active_orders;
    out << ",\"totalRestingQuantity\":" << resting_quantity;
    out << '}';
}

}  // namespace

void ReplayVisualisationWriter::write_ndjson_line(
    std::ostream& out, std::size_t index, const matching_engine::OrderCommand& command,
    const std::vector<matching_engine::EngineEvent>& events,
    const order_book::OrderBook& book) {
    write_record_body(out, index, command, events, book);
    out << '\n';
}

std::string ReplayVisualisationWriter::format_record(
    std::size_t index, const matching_engine::OrderCommand& command,
    const std::vector<matching_engine::EngineEvent>& events,
    const order_book::OrderBook& book) {
    std::ostringstream out;
    write_record_body(out, index, command, events, book);
    return out.str();
}

std::string ReplayVisualisationWriter::format_sse_frame(const std::string& json_record) {
    return "data: " + json_record + "\n\n";
}

}  // namespace viz
