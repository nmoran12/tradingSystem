#include "matching_engine/EngineEvent.hpp"
#include "matching_engine/MatchingEngine.hpp"
#include "matching_engine/OrderCommand.hpp"

#include <gtest/gtest.h>

using namespace market_data;
using namespace matching_engine;

namespace {

OrderCommand make_new_limit(uint64_t order_id, Side side, int64_t price, uint64_t quantity) {
    OrderCommand command;
    command.type = OrderCommandType::NewOrder;
    command.order_id = order_id;
    command.side = side;
    command.order_type = OrderType::Limit;
    command.price = price;
    command.quantity = quantity;
    return command;
}

OrderCommand make_cancel(uint64_t order_id) {
    OrderCommand command;
    command.type = OrderCommandType::CancelOrder;
    command.order_id = order_id;
    return command;
}

size_t count_event_type(const std::vector<EngineEvent>& events, EngineEventType type) {
    size_t count = 0;
    for (const auto& event : events) {
        if (event.type == type) {
            ++count;
        }
    }
    return count;
}

uint64_t total_trade_quantity(const std::vector<EngineEvent>& events) {
    uint64_t total = 0;
    for (const auto& event : events) {
        if (event.type == EngineEventType::Trade && event.trade) {
            total += event.trade->quantity;
        }
    }
    return total;
}

}  // namespace

TEST(MatchingEngineTest, NonCrossingBuyOrderRestsOnBidSide) {
    MatchingEngine engine;
    ASSERT_EQ(engine.process(make_new_limit(1, Side::SELL, 10055, 100)).size(), 2u);
    const auto events = engine.process(make_new_limit(2, Side::BUY, 10050, 200));
    EXPECT_EQ(count_event_type(events, EngineEventType::Trade), 0u);
    ASSERT_TRUE(engine.book().best_bid().has_value());
    EXPECT_EQ(*engine.book().best_bid(), 10050);
    EXPECT_EQ(engine.book().total_quantity_at_price(Side::BUY, 10050), 200u);
}

TEST(MatchingEngineTest, NonCrossingSellOrderRestsOnAskSide) {
    MatchingEngine engine;
    ASSERT_EQ(engine.process(make_new_limit(1, Side::BUY, 10050, 100)).size(), 2u);
    const auto events = engine.process(make_new_limit(2, Side::SELL, 10055, 150));
    EXPECT_EQ(count_event_type(events, EngineEventType::Trade), 0u);
    ASSERT_TRUE(engine.book().best_ask().has_value());
    EXPECT_EQ(*engine.book().best_ask(), 10055);
    EXPECT_EQ(engine.book().total_quantity_at_price(Side::SELL, 10055), 150u);
}

TEST(MatchingEngineTest, BuyOrderFullyFillsRestingSell) {
    MatchingEngine engine;
    ASSERT_EQ(engine.process(make_new_limit(1, Side::SELL, 10055, 100)).size(), 2u);
    const auto events = engine.process(make_new_limit(2, Side::BUY, 10055, 100));
    EXPECT_EQ(count_event_type(events, EngineEventType::Trade), 1u);
    EXPECT_EQ(total_trade_quantity(events), 100u);
    EXPECT_FALSE(engine.book().best_bid().has_value());
    EXPECT_FALSE(engine.book().best_ask().has_value());
    ASSERT_EQ(events.size(), 2u);
    ASSERT_TRUE(events[1].trade.has_value());
    EXPECT_EQ(events[1].trade->aggressive_order_id, 2u);
    EXPECT_EQ(events[1].trade->resting_order_id, 1u);
    EXPECT_EQ(events[1].trade->price, 10055);
}

TEST(MatchingEngineTest, SellOrderFullyFillsRestingBuy) {
    MatchingEngine engine;
    ASSERT_EQ(engine.process(make_new_limit(1, Side::BUY, 10050, 80)).size(), 2u);
    const auto events = engine.process(make_new_limit(2, Side::SELL, 10050, 80));
    EXPECT_EQ(count_event_type(events, EngineEventType::Trade), 1u);
    EXPECT_EQ(total_trade_quantity(events), 80u);
    EXPECT_FALSE(engine.book().contains_order(1));
    EXPECT_FALSE(engine.book().contains_order(2));
}

TEST(MatchingEngineTest, BuyOrderPartiallyFillsAndRestsRemainingQuantity) {
    MatchingEngine engine;
    ASSERT_EQ(engine.process(make_new_limit(1, Side::SELL, 10055, 50)).size(), 2u);
    const auto events = engine.process(make_new_limit(2, Side::BUY, 10055, 100));
    EXPECT_EQ(count_event_type(events, EngineEventType::Trade), 1u);
    EXPECT_EQ(total_trade_quantity(events), 50u);
    EXPECT_EQ(engine.book().total_quantity_at_price(Side::BUY, 10055), 50u);
    EXPECT_FALSE(engine.book().best_ask().has_value());
}

TEST(MatchingEngineTest, SellOrderPartiallyFillsAndRestsRemainingQuantity) {
    MatchingEngine engine;
    ASSERT_EQ(engine.process(make_new_limit(1, Side::BUY, 10050, 40)).size(), 2u);
    const auto events = engine.process(make_new_limit(2, Side::SELL, 10050, 100));
    EXPECT_EQ(count_event_type(events, EngineEventType::Trade), 1u);
    EXPECT_EQ(total_trade_quantity(events), 40u);
    EXPECT_EQ(engine.book().total_quantity_at_price(Side::SELL, 10050), 60u);
    EXPECT_FALSE(engine.book().best_bid().has_value());
}

TEST(MatchingEngineTest, BuyMatchesMultipleRestingSellsInPriceTimePriority) {
    MatchingEngine engine;
    ASSERT_EQ(engine.process(make_new_limit(1, Side::SELL, 10055, 30)).size(), 2u);
    ASSERT_EQ(engine.process(make_new_limit(2, Side::SELL, 10055, 20)).size(), 2u);
    ASSERT_EQ(engine.process(make_new_limit(3, Side::SELL, 10058, 40)).size(), 2u);

    const auto events = engine.process(make_new_limit(4, Side::BUY, 10060, 60));
    EXPECT_EQ(count_event_type(events, EngineEventType::Trade), 3u);
    EXPECT_EQ(total_trade_quantity(events), 60u);

    ASSERT_TRUE(events[1].trade.has_value());
    EXPECT_EQ(events[1].trade->resting_order_id, 1u);
    EXPECT_EQ(events[1].trade->quantity, 30u);

    ASSERT_TRUE(events[2].trade.has_value());
    EXPECT_EQ(events[2].trade->resting_order_id, 2u);
    EXPECT_EQ(events[2].trade->quantity, 20u);

    ASSERT_TRUE(events[3].trade.has_value());
    EXPECT_EQ(events[3].trade->resting_order_id, 3u);
    EXPECT_EQ(events[3].trade->quantity, 10u);
    EXPECT_EQ(engine.book().total_quantity_at_price(Side::SELL, 10058), 30u);
}

TEST(MatchingEngineTest, SellMatchesMultipleRestingBuysInPriceTimePriority) {
    MatchingEngine engine;
    ASSERT_EQ(engine.process(make_new_limit(1, Side::BUY, 10060, 25)).size(), 2u);
    ASSERT_EQ(engine.process(make_new_limit(2, Side::BUY, 10060, 15)).size(), 2u);
    ASSERT_EQ(engine.process(make_new_limit(3, Side::BUY, 10055, 50)).size(), 2u);

    const auto events = engine.process(make_new_limit(4, Side::SELL, 10055, 55));
    EXPECT_EQ(count_event_type(events, EngineEventType::Trade), 3u);
    EXPECT_EQ(total_trade_quantity(events), 55u);

    ASSERT_TRUE(events[1].trade.has_value());
    EXPECT_EQ(events[1].trade->resting_order_id, 1u);
    EXPECT_EQ(events[1].trade->price, 10060);

    ASSERT_TRUE(events[2].trade.has_value());
    EXPECT_EQ(events[2].trade->resting_order_id, 2u);

    ASSERT_TRUE(events[3].trade.has_value());
    EXPECT_EQ(events[3].trade->resting_order_id, 3u);
    EXPECT_EQ(events[3].trade->quantity, 15u);
    EXPECT_EQ(engine.book().total_quantity_at_price(Side::BUY, 10055), 35u);
}

TEST(MatchingEngineTest, FifoPriorityPreservedWithinSamePriceLevel) {
    MatchingEngine engine;
    ASSERT_EQ(engine.process(make_new_limit(10, Side::SELL, 10055, 50)).size(), 2u);
    ASSERT_EQ(engine.process(make_new_limit(11, Side::SELL, 10055, 50)).size(), 2u);

    const auto events = engine.process(make_new_limit(12, Side::BUY, 10055, 70));
    ASSERT_EQ(count_event_type(events, EngineEventType::Trade), 2u);

    ASSERT_TRUE(events[1].trade.has_value());
    EXPECT_EQ(events[1].trade->resting_order_id, 10u);
    EXPECT_EQ(events[1].trade->quantity, 50u);

    ASSERT_TRUE(events[2].trade.has_value());
    EXPECT_EQ(events[2].trade->resting_order_id, 11u);
    EXPECT_EQ(events[2].trade->quantity, 20u);
}

TEST(MatchingEngineTest, CancelRemovesRestingOrder) {
    MatchingEngine engine;
    ASSERT_EQ(engine.process(make_new_limit(1, Side::BUY, 10050, 100)).size(), 2u);
    const auto events = engine.process(make_cancel(1));
    ASSERT_EQ(events.size(), 1u);
    EXPECT_EQ(events[0].type, EngineEventType::OrderCancelled);
    EXPECT_FALSE(engine.book().contains_order(1));
    EXPECT_FALSE(engine.book().best_bid().has_value());
}

TEST(MatchingEngineTest, CancelUnknownOrderIdIsRejected) {
    MatchingEngine engine;
    const auto events = engine.process(make_cancel(99));
    ASSERT_EQ(events.size(), 1u);
    EXPECT_EQ(events[0].type, EngineEventType::OrderRejected);
    EXPECT_EQ(events[0].reason, "unknown order id");
}

TEST(MatchingEngineTest, DuplicateOrderIdIsRejected) {
    MatchingEngine engine;
    ASSERT_EQ(engine.process(make_new_limit(1, Side::BUY, 10050, 100)).size(), 2u);
    const auto events = engine.process(make_new_limit(1, Side::SELL, 10055, 50));
    ASSERT_EQ(events.size(), 1u);
    EXPECT_EQ(events[0].type, EngineEventType::OrderRejected);
    EXPECT_EQ(events[0].reason, "duplicate order id");
    EXPECT_TRUE(engine.book().contains_order(1));
    EXPECT_EQ(engine.book().total_quantity_at_price(Side::BUY, 10050), 100u);
}
