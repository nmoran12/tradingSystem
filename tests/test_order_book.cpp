#include "market_data/MarketEvent.hpp"
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

}  // namespace

TEST(OrderBookTest, AddingBuyOrderUpdatesBestBid) {
    OrderBook book;
    EXPECT_FALSE(book.best_bid().has_value());
    ASSERT_TRUE(book.add_order(make_order(1, Side::BUY, 10050, 200)));
    ASSERT_TRUE(book.best_bid().has_value());
    EXPECT_EQ(*book.best_bid(), 10050);
}

TEST(OrderBookTest, AddingSellOrderUpdatesBestAsk) {
    OrderBook book;
    EXPECT_FALSE(book.best_ask().has_value());
    ASSERT_TRUE(book.add_order(make_order(2, Side::SELL, 10055, 150)));
    ASSERT_TRUE(book.best_ask().has_value());
    EXPECT_EQ(*book.best_ask(), 10055);
}

TEST(OrderBookTest, BestBidChoosesHighestBuyPrice) {
    OrderBook book;
    ASSERT_TRUE(book.add_order(make_order(1, Side::BUY, 10050, 100)));
    ASSERT_TRUE(book.add_order(make_order(2, Side::BUY, 10060, 100)));
    EXPECT_EQ(*book.best_bid(), 10060);
}

TEST(OrderBookTest, BestAskChoosesLowestSellPrice) {
    OrderBook book;
    ASSERT_TRUE(book.add_order(make_order(1, Side::SELL, 10070, 100)));
    ASSERT_TRUE(book.add_order(make_order(2, Side::SELL, 10055, 100)));
    EXPECT_EQ(*book.best_ask(), 10055);
}

TEST(OrderBookTest, OrdersAtSamePricePreserveFifoOrder) {
    OrderBook book;
    ASSERT_TRUE(book.add_order(make_order(1, Side::BUY, 10050, 100, 1)));
    ASSERT_TRUE(book.add_order(make_order(2, Side::BUY, 10050, 200, 2)));

    MarketEvent execute_first;
    execute_first.type = EventType::EXECUTE;
    execute_first.order_id = 1;
    execute_first.quantity = 50;
    ASSERT_TRUE(book.apply_event(execute_first));

    EXPECT_EQ(book.total_quantity_at_price(Side::BUY, 10050), 250u);

    MarketEvent execute_second;
    execute_second.type = EventType::EXECUTE;
    execute_second.order_id = 2;
    execute_second.quantity = 200;
    ASSERT_TRUE(book.apply_event(execute_second));

    EXPECT_EQ(book.total_quantity_at_price(Side::BUY, 10050), 50u);
}

TEST(OrderBookTest, CancellingAnOrderRemovesIt) {
    OrderBook book;
    ASSERT_TRUE(book.add_order(make_order(1, Side::BUY, 10050, 200)));
    ASSERT_TRUE(book.cancel_order(1));
    EXPECT_FALSE(book.best_bid().has_value());
}

TEST(OrderBookTest, ExecutingPartOfAnOrderReducesQuantity) {
    OrderBook book;
    ASSERT_TRUE(book.add_order(make_order(2, Side::SELL, 10055, 150)));
    ASSERT_TRUE(book.execute_order(2, 50));
    EXPECT_EQ(book.total_quantity_at_price(Side::SELL, 10055), 100u);
    ASSERT_TRUE(book.best_ask().has_value());
}

TEST(OrderBookTest, ExecutingFullRemainingQuantityRemovesOrder) {
    OrderBook book;
    ASSERT_TRUE(book.add_order(make_order(2, Side::SELL, 10055, 150)));
    ASSERT_TRUE(book.execute_order(2, 150));
    EXPECT_FALSE(book.best_ask().has_value());
    EXPECT_EQ(book.total_quantity_at_price(Side::SELL, 10055), 0u);
}

TEST(OrderBookTest, CancellingMissingOrderReturnsFalse) {
    OrderBook book;
    EXPECT_FALSE(book.cancel_order(999));
}

TEST(OrderBookTest, ExecutingMissingOrderReturnsFalse) {
    OrderBook book;
    EXPECT_FALSE(book.execute_order(999, 10));
}

TEST(OrderBookTest, SpreadRequiresBothSides) {
    OrderBook book;
    ASSERT_TRUE(book.add_order(make_order(1, Side::BUY, 10050, 100)));
    ASSERT_TRUE(book.add_order(make_order(2, Side::SELL, 10055, 100)));
    ASSERT_TRUE(book.spread().has_value());
    EXPECT_EQ(*book.spread(), 5);
}

TEST(OrderBookTest, PeekBestAskSkipsCancelledHeadOrder) {
    OrderBook book;
    ASSERT_TRUE(book.add_order(make_order(1, Side::SELL, 10055, 100)));
    ASSERT_TRUE(book.add_order(make_order(2, Side::SELL, 10055, 200)));
    ASSERT_TRUE(book.cancel_order(1));

    const auto front = book.peek_best_ask();
    ASSERT_TRUE(front.has_value());
    EXPECT_EQ(front->order_id, 2u);
    EXPECT_TRUE(book.best_ask().has_value());
}

TEST(OrderBookTest, CancelMiddleOrderPreservesFifoForRemaining) {
    OrderBook book;
    ASSERT_TRUE(book.add_order(make_order(1, Side::SELL, 10055, 100)));
    ASSERT_TRUE(book.add_order(make_order(2, Side::SELL, 10055, 200)));
    ASSERT_TRUE(book.add_order(make_order(3, Side::SELL, 10055, 300)));
    ASSERT_TRUE(book.cancel_order(2));

    const auto front = book.peek_best_ask();
    ASSERT_TRUE(front.has_value());
    EXPECT_EQ(front->order_id, 1u);

    ASSERT_TRUE(book.execute_order(1, 100));
    const auto next = book.peek_best_ask();
    ASSERT_TRUE(next.has_value());
    EXPECT_EQ(next->order_id, 3u);
}

TEST(OrderBookTest, PartialFillThenCancelRemainder) {
    OrderBook book;
    ASSERT_TRUE(book.add_order(make_order(1, Side::BUY, 10050, 100)));
    ASSERT_TRUE(book.execute_order(1, 40));
    EXPECT_EQ(book.total_quantity_at_price(Side::BUY, 10050), 60u);
    ASSERT_TRUE(book.cancel_order(1));
    EXPECT_EQ(book.total_quantity_at_price(Side::BUY, 10050), 0u);
    EXPECT_FALSE(book.best_bid().has_value());
    EXPECT_EQ(book.active_order_count(), 0u);
}

TEST(OrderBookTest, PriceLevelRemovedWhenAllOrdersInactive) {
    OrderBook book;
    ASSERT_TRUE(book.add_order(make_order(1, Side::SELL, 10055, 50)));
    ASSERT_TRUE(book.cancel_order(1));
    EXPECT_FALSE(book.best_ask().has_value());
    EXPECT_EQ(book.total_quantity_at_price(Side::SELL, 10055), 0u);
    EXPECT_FALSE(book.validate_invariants().has_value());
}

TEST(OrderBookTest, DuplicateOrderIdRejected) {
    OrderBook book;
    ASSERT_TRUE(book.add_order(make_order(1, Side::BUY, 10050, 100)));
    EXPECT_FALSE(book.add_order(make_order(1, Side::BUY, 10060, 50)));
    EXPECT_EQ(book.active_order_count(), 1u);
}
