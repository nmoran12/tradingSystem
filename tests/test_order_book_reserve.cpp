#include "order_book/Order.hpp"
#include "order_book/OrderBook.hpp"

#include <gtest/gtest.h>

using namespace market_data;
using namespace order_book;

namespace {

Order make_order(uint64_t order_id, Side side, int64_t price, uint32_t quantity,
                 uint64_t timestamp = 1) {
    Order order;
    order.order_id = order_id;
    order.symbol = "AAPL";
    order.side = side;
    order.price = price;
    order.quantity = quantity;
    order.timestamp = timestamp;
    return order;
}

void apply_representative_sequence(OrderBook& book) {
    ASSERT_TRUE(book.add_order(make_order(1, Side::SELL, 10055, 100)));
    ASSERT_TRUE(book.add_order(make_order(2, Side::BUY, 10050, 50)));
    ASSERT_TRUE(book.add_order(make_order(3, Side::SELL, 10055, 30, 2)));
    ASSERT_TRUE(book.execute_order(1, 40));
    ASSERT_TRUE(book.cancel_order(2));
    ASSERT_TRUE(book.add_order(make_order(4, Side::BUY, 10050, 80)));
}

void expect_same_observable_state(const OrderBook& lhs, const OrderBook& rhs) {
    EXPECT_EQ(lhs.active_order_count(), rhs.active_order_count());
    EXPECT_EQ(lhs.total_resting_quantity(), rhs.total_resting_quantity());
    EXPECT_EQ(lhs.best_bid(), rhs.best_bid());
    EXPECT_EQ(lhs.best_ask(), rhs.best_ask());
    EXPECT_EQ(lhs.spread(), rhs.spread());
    EXPECT_EQ(lhs.validate_invariants(), rhs.validate_invariants());
}

}  // namespace

TEST(OrderBookReserveTest, ReservedAndUnreservedProduceSameObservableState) {
    OrderBook unreserved;
    apply_representative_sequence(unreserved);

    OrderBook reserved;
    reserved.reserve_active_orders(8);
    apply_representative_sequence(reserved);

    expect_same_observable_state(unreserved, reserved);
    EXPECT_FALSE(unreserved.validate_invariants().has_value());
    EXPECT_FALSE(reserved.validate_invariants().has_value());
}

TEST(OrderBookReserveTest, ReserveOnNonEmptyBookDoesNotChangeExistingOrders) {
    OrderBook book;
    ASSERT_TRUE(book.add_order(make_order(1, Side::BUY, 10050, 100)));
    const auto bid_before = book.best_bid();
    const size_t count_before = book.active_order_count();

    book.reserve_active_orders(1024);

    EXPECT_EQ(book.active_order_count(), count_before);
    EXPECT_EQ(book.best_bid(), bid_before);
    EXPECT_FALSE(book.validate_invariants().has_value());
}
