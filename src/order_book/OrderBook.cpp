#include "order_book/OrderBook.hpp"

#include <iostream>

namespace order_book {
namespace {

Order order_from_event(const market_data::MarketEvent& event) {
    Order order;
    order.order_id = event.order_id;
    order.symbol = event.symbol;
    order.side = event.side;
    order.price = event.price;
    order.quantity = event.quantity;
    order.timestamp = event.timestamp;
    return order;
}

}  // namespace

bool OrderBook::add_order(const Order& order) {
    if (order.side != market_data::Side::BUY && order.side != market_data::Side::SELL) {
        return false;
    }
    if (order_lookup_.contains(order.order_id)) {
        return false;
    }
    return add_order_to_side(order.side, order);
}

bool OrderBook::add_order_to_side(market_data::Side side, const Order& order) {
    if (side == market_data::Side::BUY) {
        auto& level = buy_book_[order.price];
        level.push_back(order);
        order_lookup_[order.order_id] = OrderLocation{side, order.price, std::prev(level.end())};
        return true;
    }

    auto& level = sell_book_[order.price];
    level.push_back(order);
    order_lookup_[order.order_id] = OrderLocation{side, order.price, std::prev(level.end())};
    return true;
}

bool OrderBook::cancel_order(uint64_t order_id) {
    const auto location_it = order_lookup_.find(order_id);
    if (location_it == order_lookup_.end()) {
        return false;
    }

    const auto location = location_it->second;
    remove_order_at_location(location);
    order_lookup_.erase(location_it);
    return true;
}

bool OrderBook::execute_order(uint64_t order_id, uint32_t quantity) {
    if (quantity == 0) {
        return false;
    }

    const auto location_it = order_lookup_.find(order_id);
    if (location_it == order_lookup_.end()) {
        return false;
    }

    auto location = location_it->second;
    if (location.it->quantity < quantity) {
        return false;
    }

    location.it->quantity -= quantity;
    if (location.it->quantity == 0) {
        remove_order_at_location(location);
        order_lookup_.erase(location_it);
    }

    return true;
}

void OrderBook::remove_order_at_location(const OrderLocation& location) {
    if (location.side == market_data::Side::BUY) {
        auto level_it = buy_book_.find(location.price);
        if (level_it == buy_book_.end()) {
            return;
        }
        level_it->second.erase(location.it);
        if (level_it->second.empty()) {
            buy_book_.erase(level_it);
        }
        return;
    }

    auto level_it = sell_book_.find(location.price);
    if (level_it == sell_book_.end()) {
        return;
    }
    level_it->second.erase(location.it);
    if (level_it->second.empty()) {
        sell_book_.erase(level_it);
    }
}

bool OrderBook::contains_order(uint64_t order_id) const {
    return order_lookup_.contains(order_id);
}

std::optional<Order> OrderBook::peek_best_bid() const {
    for (const auto& [price, orders] : buy_book_) {
        (void)price;
        if (!orders.empty()) {
            return orders.front();
        }
    }
    return std::nullopt;
}

std::optional<Order> OrderBook::peek_best_ask() const {
    for (const auto& [price, orders] : sell_book_) {
        (void)price;
        if (!orders.empty()) {
            return orders.front();
        }
    }
    return std::nullopt;
}

bool OrderBook::apply_event(const market_data::MarketEvent& event) {
    switch (event.type) {
        case market_data::EventType::ADD:
            return add_order(order_from_event(event));
        case market_data::EventType::CANCEL:
            return cancel_order(event.order_id);
        case market_data::EventType::EXECUTE:
            return execute_order(event.order_id, event.quantity);
    }
    return false;
}

std::optional<int64_t> OrderBook::best_bid() const {
    for (const auto& [price, orders] : buy_book_) {
        if (!orders.empty()) {
            return price;
        }
    }
    return std::nullopt;
}

std::optional<int64_t> OrderBook::best_ask() const {
    for (const auto& [price, orders] : sell_book_) {
        if (!orders.empty()) {
            return price;
        }
    }
    return std::nullopt;
}

std::optional<int64_t> OrderBook::spread() const {
    const auto bid = best_bid();
    const auto ask = best_ask();
    if (!bid || !ask) {
        return std::nullopt;
    }
    return *ask - *bid;
}

uint32_t OrderBook::total_quantity_at_price(market_data::Side side, int64_t price) const {
    auto accumulate_level = [](const auto& book, int64_t level_price) -> uint32_t {
        const auto level_it = book.find(level_price);
        if (level_it == book.end()) {
            return 0;
        }
        uint32_t total = 0;
        for (const auto& order : level_it->second) {
            total += order.quantity;
        }
        return total;
    };

    if (side == market_data::Side::BUY) {
        return accumulate_level(buy_book_, price);
    }
    return accumulate_level(sell_book_, price);
}

std::optional<Order> OrderBook::get_order(uint64_t order_id) const {
    const auto location_it = order_lookup_.find(order_id);
    if (location_it == order_lookup_.end()) {
        return std::nullopt;
    }
    return *(location_it->second.it);
}

size_t OrderBook::active_order_count() const { return order_lookup_.size(); }

uint64_t OrderBook::total_resting_quantity() const {
    auto sum_book = [](const auto& book) -> uint64_t {
        uint64_t total = 0;
        for (const auto& [price, orders] : book) {
            (void)price;
            for (const auto& order : orders) {
                total += order.quantity;
            }
        }
        return total;
    };
    return sum_book(buy_book_) + sum_book(sell_book_);
}

std::optional<std::string_view> OrderBook::validate_invariants() const {
    size_t book_order_count = 0;
    for (const auto& [price, orders] : buy_book_) {
        (void)price;
        book_order_count += orders.size();
    }
    for (const auto& [price, orders] : sell_book_) {
        (void)price;
        book_order_count += orders.size();
    }
    if (book_order_count != order_lookup_.size()) {
        return "order lookup mismatch";
    }

    const auto bid = best_bid();
    const auto ask = best_ask();
    if (bid && ask && *bid >= *ask) {
        return "crossed book";
    }

    auto validate_book = [](const auto& book) -> std::optional<std::string_view> {
        for (const auto& [price, orders] : book) {
            for (const auto& order : orders) {
                if (order.quantity == 0) {
                    return "non-positive resting quantity";
                }
                if (order.price != price) {
                    return "order price mismatch at level";
                }
            }
        }
        return std::nullopt;
    };

    if (const auto error = validate_book(buy_book_)) {
        return error;
    }
    if (const auto error = validate_book(sell_book_)) {
        return error;
    }

    if (total_resting_quantity() == 0 && !order_lookup_.empty()) {
        return "active orders with zero total quantity";
    }

    return std::nullopt;
}

void OrderBook::print_depth(std::ostream& out, size_t levels) const {
    out << "Order book depth:\n";
    out << "Bids:\n";

    size_t bid_levels = 0;
    for (const auto& [price, orders] : buy_book_) {
        if (bid_levels >= levels) {
            break;
        }
        uint32_t total = 0;
        for (const auto& order : orders) {
            total += order.quantity;
        }
        if (total > 0) {
            out << price << " x " << total << '\n';
            ++bid_levels;
        }
    }

    out << "Asks:\n";
    size_t ask_levels = 0;
    for (const auto& [price, orders] : sell_book_) {
        if (ask_levels >= levels) {
            break;
        }
        uint32_t total = 0;
        for (const auto& order : orders) {
            total += order.quantity;
        }
        if (total > 0) {
            out << price << " x " << total << '\n';
            ++ask_levels;
        }
    }
}

}  // namespace order_book
