#include "matching_engine/EngineEvent.hpp"
#include "matching_engine/MatchingEngine.hpp"
#include "matching_engine/OrderCommand.hpp"

#include <gtest/gtest.h>

using namespace market_data;
using namespace matching_engine;

namespace {

bool events_equal(const std::vector<EngineEvent>& lhs, const std::vector<EngineEvent>& rhs) {
    if (lhs.size() != rhs.size()) {
        return false;
    }
    for (size_t i = 0; i < lhs.size(); ++i) {
        const auto& a = lhs[i];
        const auto& b = rhs[i];
        if (a.type != b.type || a.order_id != b.order_id || a.reason != b.reason) {
            return false;
        }
        if (static_cast<bool>(a.trade) != static_cast<bool>(b.trade)) {
            return false;
        }
        if (a.trade) {
            if (!b.trade || a.trade->aggressive_order_id != b.trade->aggressive_order_id ||
                a.trade->resting_order_id != b.trade->resting_order_id ||
                a.trade->price != b.trade->price || a.trade->quantity != b.trade->quantity) {
                return false;
            }
        }
    }
    return true;
}

void expect_process_equals_process_into(MatchingEngine& process_engine,
                                        MatchingEngine& into_engine,
                                        const OrderCommand& command,
                                        std::vector<EngineEvent>& scratch) {
    const auto from_process = process_engine.process(command);
    into_engine.process_into(command, scratch);
    EXPECT_TRUE(events_equal(from_process, scratch)) << "order_id=" << command.order_id;
}

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

OrderCommand make_new_market(uint64_t order_id, Side side, uint64_t quantity) {
    OrderCommand command;
    command.type = OrderCommandType::NewOrder;
    command.order_id = order_id;
    command.side = side;
    command.order_type = OrderType::Market;
    command.price = 0;
    command.quantity = quantity;
    return command;
}

OrderCommand make_cancel(uint64_t order_id) {
    OrderCommand command;
    command.type = OrderCommandType::CancelOrder;
    command.order_id = order_id;
    return command;
}

OrderCommand make_modify(uint64_t order_id, Side side, int64_t price, uint64_t quantity) {
    OrderCommand command;
    command.type = OrderCommandType::ModifyOrder;
    command.order_id = order_id;
    command.side = side;
    command.order_type = OrderType::Limit;
    command.price = price;
    command.quantity = quantity;
    return command;
}

}  // namespace

TEST(MatchingEngineProcessIntoTest, MatchesProcessForRepresentativeCommands) {
    MatchingEngine process_engine;
    MatchingEngine into_engine;
    std::vector<EngineEvent> scratch;

    expect_process_equals_process_into(process_engine, into_engine,
                                       make_new_limit(1, Side::BUY, 10050, 100), scratch);
    expect_process_equals_process_into(process_engine, into_engine,
                                       make_new_limit(2, Side::SELL, 10055, 100), scratch);

    process_engine.process(make_new_limit(3, Side::SELL, 10055, 80));
    into_engine.process_into(make_new_limit(3, Side::SELL, 10055, 80), scratch);
    expect_process_equals_process_into(process_engine, into_engine,
                                       make_new_market(4, Side::BUY, 80), scratch);

    expect_process_equals_process_into(process_engine, into_engine, make_cancel(1), scratch);
    expect_process_equals_process_into(process_engine, into_engine, make_cancel(99), scratch);

    process_engine.process(make_new_limit(5, Side::SELL, 10070, 50));
    into_engine.process_into(make_new_limit(5, Side::SELL, 10070, 50), scratch);
    expect_process_equals_process_into(process_engine, into_engine,
                                       make_modify(5, Side::SELL, 10080, 50), scratch);

    expect_process_equals_process_into(process_engine, into_engine,
                                       make_new_limit(5, Side::BUY, 10050, 10), scratch);
}

TEST(MatchingEngineProcessIntoTest, RepeatedCallsClearPreviousEvents) {
    MatchingEngine engine;
    std::vector<EngineEvent> scratch;

    engine.process_into(make_new_limit(1, Side::BUY, 10050, 100), scratch);
    ASSERT_EQ(scratch.size(), 2u);

    engine.process_into(make_cancel(1), scratch);
    ASSERT_EQ(scratch.size(), 1u);
    EXPECT_EQ(scratch.front().type, EngineEventType::OrderCancelled);

    engine.process_into(make_cancel(99), scratch);
    ASSERT_EQ(scratch.size(), 1u);
    EXPECT_EQ(scratch.front().type, EngineEventType::OrderRejected);
}

TEST(MatchingEngineProcessIntoTest, ReusesCapacityWithoutLeakingPriorEvents) {
    MatchingEngine engine;
    std::vector<EngineEvent> scratch;
    scratch.reserve(16);

    engine.process_into(make_new_limit(1, Side::SELL, 10055, 30), scratch);
    engine.process_into(make_new_limit(2, Side::SELL, 10055, 20), scratch);
    engine.process_into(make_new_limit(3, Side::SELL, 10058, 40), scratch);
    const size_t capacity_after_warmup = scratch.capacity();
    EXPECT_GE(capacity_after_warmup, 2u);

    const auto multi_match_cmd = make_new_limit(4, Side::BUY, 10060, 60);
    MatchingEngine expected_engine;
    expected_engine.process(make_new_limit(1, Side::SELL, 10055, 30));
    expected_engine.process(make_new_limit(2, Side::SELL, 10055, 20));
    expected_engine.process(make_new_limit(3, Side::SELL, 10058, 40));
    const auto expected = expected_engine.process(multi_match_cmd);

    engine.process_into(multi_match_cmd, scratch);
    EXPECT_GE(scratch.capacity(), capacity_after_warmup);
    EXPECT_TRUE(events_equal(expected, scratch));
}

TEST(MatchingEngineProcessIntoTest, ProcessReturnRemainsIndependent) {
    MatchingEngine engine;
    auto first = engine.process(make_new_limit(1, Side::BUY, 10050, 100));
    auto second = engine.process(make_new_limit(2, Side::SELL, 10055, 50));

    first.clear();
    EXPECT_EQ(second.size(), 2u);
    EXPECT_EQ(second.front().type, EngineEventType::OrderAccepted);
}
