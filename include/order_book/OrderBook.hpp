#pragma once

#include "market_data/MarketEvent.hpp"
#include "order_book/Order.hpp"

#include <cstddef>
#include <cstdint>
#include <list>
#include <map>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <unordered_map>

namespace order_book {

class OrderBook {
public:
    /// Pre-size @c order_lookup_ for at least @p expected_active_orders entries.
    /// Does not reserve list/map nodes; safe to call on an empty or non-empty book.
    /// Reduces rehash during growth when the peak active order count is known approximately.
    void reserve_active_orders(size_t expected_active_orders);

    bool add_order(const Order& order);
    bool cancel_order(uint64_t order_id);
    bool execute_order(uint64_t order_id, uint32_t quantity);
    bool apply_event(const market_data::MarketEvent& event);

    bool remove_order(uint64_t order_id) { return cancel_order(order_id); }
    bool reduce_order(uint64_t order_id, uint32_t quantity) {
        return execute_order(order_id, quantity);
    }

    [[nodiscard]] bool contains_order(uint64_t order_id) const;
    [[nodiscard]] std::optional<Order> peek_best_bid() const;
    [[nodiscard]] std::optional<Order> peek_best_ask() const;

    [[nodiscard]] std::optional<int64_t> best_bid() const;
    [[nodiscard]] std::optional<int64_t> best_ask() const;
    [[nodiscard]] std::optional<int64_t> spread() const;
    [[nodiscard]] uint32_t total_quantity_at_price(market_data::Side side,
                                                   int64_t price) const;
    [[nodiscard]] std::optional<Order> get_order(uint64_t order_id) const;
    [[nodiscard]] size_t active_order_count() const;
    [[nodiscard]] uint64_t total_resting_quantity() const;
    [[nodiscard]] std::optional<std::string_view> validate_invariants() const;

    void print_depth(std::ostream& out, size_t levels) const;

private:
    struct OrderLocation {
        market_data::Side side{market_data::Side::UNKNOWN};
        int64_t price{};
        std::list<Order>::iterator it;
    };

    using BuyBook = std::map<int64_t, std::list<Order>, std::greater<int64_t>>;
    using SellBook = std::map<int64_t, std::list<Order>>;

    bool add_order_to_side(market_data::Side side, const Order& order);
    void remove_order_at_location(const OrderLocation& location);

    BuyBook buy_book_;
    SellBook sell_book_;
    std::unordered_map<uint64_t, OrderLocation> order_lookup_;
};

}  // namespace order_book
