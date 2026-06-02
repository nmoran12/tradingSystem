#pragma once

#include "market_data/MarketEvent.hpp"
#include "order_book/Order.hpp"

#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace order_book {

/// Experimental contiguous price level (see branch experiment/vector-price-levels).
struct OrderSlot {
    Order order;
    bool active{false};
};

struct PriceLevel {
    std::vector<OrderSlot> orders;
    std::size_t head{0};

    std::size_t append(Order order);
    void mark_inactive(std::size_t index);
    void compact_head();
    [[nodiscard]] bool has_active_orders() const;
    [[nodiscard]] std::optional<Order> front_active() const;
    [[nodiscard]] uint32_t total_active_quantity() const;
    [[nodiscard]] std::size_t active_order_count() const;
};

class OrderBook {
public:
    /// Pre-size @c order_lookup_ for at least @p expected_active_orders entries.
    /// Does not reserve per-price level vectors; safe on empty or non-empty book.
    void reserve_active_orders(size_t expected_active_orders);

    bool add_order(Order order);
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
        std::size_t index{};
    };

    using BuyBook = std::map<int64_t, PriceLevel, std::greater<int64_t>>;
    using SellBook = std::map<int64_t, PriceLevel>;

    bool add_order_to_side(market_data::Side side, Order order);
    void remove_order_at_location(const OrderLocation& location);
    [[nodiscard]] const PriceLevel* find_level(market_data::Side side, int64_t price) const;
    PriceLevel* find_level_mut(market_data::Side side, int64_t price);
    void erase_level_if_empty(market_data::Side side, int64_t price);

    BuyBook buy_book_;
    SellBook sell_book_;
    std::unordered_map<uint64_t, OrderLocation> order_lookup_;
};

}  // namespace order_book
