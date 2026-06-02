#include "order_book/OrderBook.hpp"

#include <iostream>
#include <utility>

namespace order_book {

std::size_t PriceLevel::append(Order order) {
    orders.push_back(OrderSlot{std::move(order), true});
    return orders.size() - 1;
}

void PriceLevel::mark_inactive(std::size_t index) {
    if (index < orders.size()) {
        orders[index].active = false;
    }
}

void PriceLevel::compact_head() {
    while (head < orders.size() && !orders[head].active) {
        ++head;
    }
}

bool PriceLevel::has_active_orders() const {
    for (std::size_t i = head; i < orders.size(); ++i) {
        if (orders[i].active && orders[i].order.quantity > 0) {
            return true;
        }
    }
    return false;
}

std::optional<Order> PriceLevel::front_active() const {
    for (std::size_t i = head; i < orders.size(); ++i) {
        if (orders[i].active && orders[i].order.quantity > 0) {
            return orders[i].order;
        }
    }
    return std::nullopt;
}

uint32_t PriceLevel::total_active_quantity() const {
    uint32_t total = 0;
    for (std::size_t i = head; i < orders.size(); ++i) {
        if (orders[i].active) {
            total += orders[i].order.quantity;
        }
    }
    return total;
}

std::size_t PriceLevel::active_order_count() const {
    std::size_t count = 0;
    for (std::size_t i = head; i < orders.size(); ++i) {
        if (orders[i].active && orders[i].order.quantity > 0) {
            ++count;
        }
    }
    return count;
}

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

void OrderBook::reserve_active_orders(size_t expected_active_orders) {
    order_lookup_.reserve(expected_active_orders);
}

const PriceLevel* OrderBook::find_level(market_data::Side side, int64_t price) const {
    if (side == market_data::Side::BUY) {
        const auto it = buy_book_.find(price);
        return it == buy_book_.end() ? nullptr : &it->second;
    }
    const auto it = sell_book_.find(price);
    return it == sell_book_.end() ? nullptr : &it->second;
}

PriceLevel* OrderBook::find_level_mut(market_data::Side side, int64_t price) {
    if (side == market_data::Side::BUY) {
        const auto it = buy_book_.find(price);
        return it == buy_book_.end() ? nullptr : &it->second;
    }
    const auto it = sell_book_.find(price);
    return it == sell_book_.end() ? nullptr : &it->second;
}

void OrderBook::erase_level_if_empty(market_data::Side side, int64_t price) {
    if (side == market_data::Side::BUY) {
        const auto it = buy_book_.find(price);
        if (it != buy_book_.end() && !it->second.has_active_orders()) {
            buy_book_.erase(it);
        }
        return;
    }
    const auto it = sell_book_.find(price);
    if (it != sell_book_.end() && !it->second.has_active_orders()) {
        sell_book_.erase(it);
    }
}

bool OrderBook::add_order(Order order) {
    if (order.side != market_data::Side::BUY && order.side != market_data::Side::SELL) {
        return false;
    }
    if (order_lookup_.contains(order.order_id)) {
        return false;
    }
    return add_order_to_side(order.side, std::move(order));
}

bool OrderBook::add_order_to_side(market_data::Side side, Order order) {
    const uint64_t order_id = order.order_id;
    const int64_t price = order.price;

    if (side == market_data::Side::BUY) {
        auto& level = buy_book_[price];
        const std::size_t index = level.append(std::move(order));
        order_lookup_.emplace(order_id, OrderLocation{side, price, index});
        return true;
    }

    auto& level = sell_book_[price];
    const std::size_t index = level.append(std::move(order));
    order_lookup_.emplace(order_id, OrderLocation{side, price, index});
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
    PriceLevel* level = find_level_mut(location.side, location.price);
    if (level == nullptr || location.index >= level->orders.size() ||
        !level->orders[location.index].active) {
        return false;
    }

    OrderSlot& slot = level->orders[location.index];
    if (slot.order.quantity < quantity) {
        return false;
    }

    slot.order.quantity -= quantity;
    if (slot.order.quantity == 0) {
        remove_order_at_location(location);
        order_lookup_.erase(location_it);
    }

    return true;
}

void OrderBook::remove_order_at_location(const OrderLocation& location) {
    PriceLevel* level = find_level_mut(location.side, location.price);
    if (level == nullptr || location.index >= level->orders.size()) {
        return;
    }

    level->mark_inactive(location.index);
    level->compact_head();
    erase_level_if_empty(location.side, location.price);
}

bool OrderBook::contains_order(uint64_t order_id) const {
    return order_lookup_.contains(order_id);
}

std::optional<Order> OrderBook::peek_best_bid() const {
    for (const auto& [price, level] : buy_book_) {
        (void)price;
        if (const auto front = level.front_active()) {
            return front;
        }
    }
    return std::nullopt;
}

std::optional<Order> OrderBook::peek_best_ask() const {
    for (const auto& [price, level] : sell_book_) {
        (void)price;
        if (const auto front = level.front_active()) {
            return front;
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
    for (const auto& [price, level] : buy_book_) {
        if (level.has_active_orders()) {
            return price;
        }
    }
    return std::nullopt;
}

std::optional<int64_t> OrderBook::best_ask() const {
    for (const auto& [price, level] : sell_book_) {
        if (level.has_active_orders()) {
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
    const PriceLevel* level = find_level(side, price);
    if (level == nullptr) {
        return 0;
    }
    return level->total_active_quantity();
}

std::optional<Order> OrderBook::get_order(uint64_t order_id) const {
    const auto location_it = order_lookup_.find(order_id);
    if (location_it == order_lookup_.end()) {
        return std::nullopt;
    }

    const OrderLocation& location = location_it->second;
    const PriceLevel* level = find_level(location.side, location.price);
    if (level == nullptr || location.index >= level->orders.size()) {
        return std::nullopt;
    }

    const OrderSlot& slot = level->orders[location.index];
    if (!slot.active) {
        return std::nullopt;
    }
    return slot.order;
}

size_t OrderBook::active_order_count() const { return order_lookup_.size(); }

uint64_t OrderBook::total_resting_quantity() const {
    auto sum_book = [](const auto& book) -> uint64_t {
        uint64_t total = 0;
        for (const auto& [price, level] : book) {
            (void)price;
            total += level.total_active_quantity();
        }
        return total;
    };
    return sum_book(buy_book_) + sum_book(sell_book_);
}

std::optional<std::string_view> OrderBook::validate_invariants() const {
    size_t active_in_book = 0;
    for (const auto& [price, level] : buy_book_) {
        (void)price;
        active_in_book += level.active_order_count();
    }
    for (const auto& [price, level] : sell_book_) {
        (void)price;
        active_in_book += level.active_order_count();
    }
    if (active_in_book != order_lookup_.size()) {
        return "order lookup mismatch";
    }

    const auto bid = best_bid();
    const auto ask = best_ask();
    if (bid && ask && *bid >= *ask) {
        return "crossed book";
    }

    auto validate_book = [](const auto& book) -> std::optional<std::string_view> {
        for (const auto& [price, level] : book) {
            for (std::size_t i = level.head; i < level.orders.size(); ++i) {
                const OrderSlot& slot = level.orders[i];
                if (!slot.active) {
                    continue;
                }
                if (slot.order.quantity == 0) {
                    return "non-positive resting quantity";
                }
                if (slot.order.price != price) {
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
    for (const auto& [price, level] : buy_book_) {
        if (bid_levels >= levels) {
            break;
        }
        const uint32_t total = level.total_active_quantity();
        if (total > 0) {
            out << price << " x " << total << '\n';
            ++bid_levels;
        }
    }

    out << "Asks:\n";
    size_t ask_levels = 0;
    for (const auto& [price, level] : sell_book_) {
        if (ask_levels >= levels) {
            break;
        }
        const uint32_t total = level.total_active_quantity();
        if (total > 0) {
            out << price << " x " << total << '\n';
            ++ask_levels;
        }
    }
}

}  // namespace order_book
