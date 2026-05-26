#include "matching_engine/MatchingEngine.hpp"
#include "matching_engine/OrderCommand.hpp"

#include <gtest/gtest.h>

using namespace market_data;
using namespace matching_engine;

namespace {

OrderCommand make_new(uint64_t id, Side side, OrderType type, int64_t price, uint64_t qty) {
    OrderCommand command;
    command.type = OrderCommandType::NewOrder;
    command.order_id = id;
    command.side = side;
    command.order_type = type;
    command.price = price;
    command.quantity = qty;
    return command;
}

OrderCommand make_modify(uint64_t id, Side side, int64_t price, uint64_t qty) {
    OrderCommand command;
    command.type = OrderCommandType::ModifyOrder;
    command.order_id = id;
    command.side = side;
    command.order_type = OrderType::Limit;
    command.price = price;
    command.quantity = qty;
    return command;
}

OrderCommand make_cancel(uint64_t id) {
    OrderCommand command;
    command.type = OrderCommandType::CancelOrder;
    command.order_id = id;
    return command;
}

size_t count_trades(const std::vector<EngineEvent>& events) {
    size_t count = 0;
    for (const auto& event : events) {
        if (event.type == EngineEventType::Trade) {
            ++count;
        }
    }
    return count;
}

bool has_event(const std::vector<EngineEvent>& events, EngineEventType type) {
    for (const auto& event : events) {
        if (event.type == type) {
            return true;
        }
    }
    return false;
}

}  // namespace

TEST(MarketOrderTest, MarketBuyFullyFillsFromAskSide) {
    MatchingEngine engine;
    ASSERT_EQ(engine.process(make_new(1, Side::SELL, OrderType::Limit, 10055, 80)).size(), 2u);
    const auto events = engine.process(make_new(2, Side::BUY, OrderType::Market, 0, 80));
    EXPECT_EQ(count_trades(events), 1u);
    EXPECT_FALSE(engine.book().contains_order(2));
    EXPECT_FALSE(engine.book().best_ask().has_value());
}

TEST(MarketOrderTest, MarketSellFullyFillsFromBidSide) {
    MatchingEngine engine;
    ASSERT_EQ(engine.process(make_new(1, Side::BUY, OrderType::Limit, 10050, 60)).size(), 2u);
    const auto events = engine.process(make_new(2, Side::SELL, OrderType::Market, 0, 60));
    EXPECT_EQ(count_trades(events), 1u);
    EXPECT_FALSE(engine.book().contains_order(2));
    EXPECT_FALSE(engine.book().best_bid().has_value());
}

TEST(MarketOrderTest, MarketBuyPartiallyFillsThenCancelsRemainder) {
    MatchingEngine engine;
    ASSERT_EQ(engine.process(make_new(1, Side::SELL, OrderType::Limit, 10055, 30)).size(), 2u);
    const auto events = engine.process(make_new(2, Side::BUY, OrderType::Market, 0, 50));
    EXPECT_EQ(count_trades(events), 1u);
    EXPECT_TRUE(has_event(events, EngineEventType::OrderCancelled));
    EXPECT_FALSE(engine.book().contains_order(2));
}

TEST(MarketOrderTest, MarketSellPartiallyFillsThenCancelsRemainder) {
    MatchingEngine engine;
    ASSERT_EQ(engine.process(make_new(1, Side::BUY, OrderType::Limit, 10060, 20)).size(), 2u);
    const auto events = engine.process(make_new(2, Side::SELL, OrderType::Market, 0, 45));
    EXPECT_EQ(count_trades(events), 1u);
    EXPECT_TRUE(has_event(events, EngineEventType::OrderCancelled));
}

TEST(MarketOrderTest, MarketOrderAgainstEmptyBookProducesNoTradeAndDoesNotRest) {
    MatchingEngine engine;
    const auto events = engine.process(make_new(1, Side::BUY, OrderType::Market, 0, 10));
    EXPECT_EQ(count_trades(events), 0u);
    EXPECT_TRUE(has_event(events, EngineEventType::OrderCancelled));
    EXPECT_FALSE(engine.book().contains_order(1));
}

TEST(MarketOrderTest, MarketOrderWithInvalidQuantityIsRejected) {
    MatchingEngine engine;
    const auto events = engine.process(make_new(1, Side::BUY, OrderType::Market, 0, 0));
    EXPECT_EQ(events[0].type, EngineEventType::OrderRejected);
}

TEST(ModifyOrderTest, ModifyUnknownOrderIsRejected) {
    MatchingEngine engine;
    const auto events = engine.process(make_modify(99, Side::BUY, 10050, 10));
    EXPECT_EQ(events[0].type, EngineEventType::OrderRejected);
}

TEST(ModifyOrderTest, ModifyRestingOrderQuantity) {
    MatchingEngine engine;
    ASSERT_EQ(engine.process(make_new(1, Side::BUY, OrderType::Limit, 10050, 100)).size(), 2u);
    const auto events = engine.process(make_modify(1, Side::BUY, 10050, 40));
    EXPECT_TRUE(has_event(events, EngineEventType::OrderAccepted));
    EXPECT_EQ(engine.book().total_quantity_at_price(Side::BUY, 10050), 40u);
}

TEST(ModifyOrderTest, ModifyRestingOrderPrice) {
    MatchingEngine engine;
    ASSERT_EQ(engine.process(make_new(1, Side::SELL, OrderType::Limit, 10070, 50)).size(), 2u);
    const auto events = engine.process(make_modify(1, Side::SELL, 10080, 50));
    EXPECT_TRUE(has_event(events, EngineEventType::OrderAccepted));
    EXPECT_EQ(*engine.book().best_ask(), 10080);
}

TEST(ModifyOrderTest, ModifyOrderToCrossingPriceCausesTrade) {
    MatchingEngine engine;
    ASSERT_EQ(engine.process(make_new(1, Side::SELL, OrderType::Limit, 10055, 100)).size(), 2u);
    ASSERT_EQ(engine.process(make_new(2, Side::BUY, OrderType::Limit, 10050, 50)).size(), 2u);
    const auto events = engine.process(make_modify(2, Side::BUY, 10060, 50));
    EXPECT_GE(count_trades(events), 1u);
    EXPECT_FALSE(engine.book().contains_order(2));
    EXPECT_TRUE(engine.book().contains_order(1));
    EXPECT_EQ(engine.book().total_quantity_at_price(Side::SELL, 10055), 50u);
}

TEST(ModifyOrderTest, ModifyDoesNotAllowSideChange) {
    MatchingEngine engine;
    ASSERT_EQ(engine.process(make_new(1, Side::BUY, OrderType::Limit, 10050, 100)).size(), 2u);
    const auto events = engine.process(make_modify(1, Side::SELL, 10050, 100));
    EXPECT_EQ(events[0].type, EngineEventType::OrderRejected);
    EXPECT_TRUE(engine.book().contains_order(1));
}

TEST(ModifyOrderTest, DuplicateOrderIdStillRejectedOnNewOrder) {
    MatchingEngine engine;
    ASSERT_EQ(engine.process(make_new(1, Side::BUY, OrderType::Limit, 10050, 100)).size(), 2u);
    const auto events = engine.process(make_new(1, Side::SELL, OrderType::Limit, 10055, 50));
    EXPECT_EQ(events[0].type, EngineEventType::OrderRejected);
}
