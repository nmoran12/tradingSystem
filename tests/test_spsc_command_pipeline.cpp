#include "matching_engine/EngineEvent.hpp"
#include "matching_engine/MatchingEngine.hpp"
#include "matching_engine/OrderCommand.hpp"
#include "pipeline/SpscCommandPipeline.hpp"

#include <gtest/gtest.h>

#include <optional>
#include <vector>

using namespace market_data;
using namespace matching_engine;
using pipeline::SpscCommandPipeline;

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

void expect_books_equivalent(const MatchingEngine& lhs, const MatchingEngine& rhs) {
    const auto& left_book = lhs.book();
    const auto& right_book = rhs.book();

    EXPECT_EQ(left_book.active_order_count(), right_book.active_order_count());
    EXPECT_EQ(left_book.best_bid(), right_book.best_bid());
    EXPECT_EQ(left_book.best_ask(), right_book.best_ask());
    EXPECT_EQ(left_book.total_resting_quantity(), right_book.total_resting_quantity());
    EXPECT_EQ(left_book.validate_invariants(), right_book.validate_invariants());
}

std::vector<EngineEvent> process_direct(MatchingEngine& engine,
                                        const std::vector<OrderCommand>& commands,
                                        std::vector<EngineEvent>& scratch) {
    std::vector<EngineEvent> all_events;
    for (const auto& command : commands) {
        engine.process_into(command, scratch);
        all_events.insert(all_events.end(), scratch.begin(), scratch.end());
    }
    return all_events;
}

std::vector<EngineEvent> process_via_pipeline(SpscCommandPipeline& pipeline,
                                              MatchingEngine& engine,
                                              const std::vector<OrderCommand>& commands,
                                              std::vector<EngineEvent>& scratch) {
    std::vector<EngineEvent> all_events;
    if (!pipeline.run_sequence(engine, commands, scratch, all_events)) {
        ADD_FAILURE() << "pipeline.run_sequence failed";
        return all_events;
    }
    EXPECT_TRUE(pipeline.empty());
    return all_events;
}

void expect_pipeline_matches_direct(const std::vector<OrderCommand>& commands,
                                    size_t queue_capacity) {
    MatchingEngine direct_engine;
    MatchingEngine pipeline_engine;
    SpscCommandPipeline pipeline(queue_capacity);
    std::vector<EngineEvent> scratch;
    scratch.reserve(8);

    const auto direct_events = process_direct(direct_engine, commands, scratch);
    const auto pipeline_events =
        process_via_pipeline(pipeline, pipeline_engine, commands, scratch);

    EXPECT_TRUE(events_equal(direct_events, pipeline_events));
    expect_books_equivalent(direct_engine, pipeline_engine);
    EXPECT_FALSE(direct_engine.book().validate_invariants().has_value());
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

OrderCommand make_cancel(uint64_t order_id) {
    OrderCommand command;
    command.type = OrderCommandType::CancelOrder;
    command.order_id = order_id;
    return command;
}

}  // namespace

TEST(SpscCommandPipelineTest, AddOnlySequenceMatchesDirectProcessing) {
    const std::vector<OrderCommand> commands = {
        make_new_limit(1, Side::BUY, 10050, 100),
        make_new_limit(2, Side::SELL, 10055, 80),
        make_new_limit(3, Side::BUY, 10048, 50),
    };

    expect_pipeline_matches_direct(commands, 8);
}

TEST(SpscCommandPipelineTest, AddAndCancelSequenceMatchesDirectProcessing) {
    const std::vector<OrderCommand> commands = {
        make_new_limit(1, Side::BUY, 10050, 100),
        make_new_limit(2, Side::SELL, 10055, 80),
        make_cancel(1),
        make_cancel(99),
    };

    expect_pipeline_matches_direct(commands, 4);
}

TEST(SpscCommandPipelineTest, MatchingSequenceWithTradesMatchesDirectProcessing) {
    const std::vector<OrderCommand> commands = {
        make_new_limit(1, Side::SELL, 10055, 30),
        make_new_limit(2, Side::SELL, 10055, 20),
        make_new_limit(3, Side::SELL, 10058, 40),
        make_new_limit(4, Side::BUY, 10060, 60),
        make_cancel(2),
    };

    expect_pipeline_matches_direct(commands, 16);
}

TEST(SpscCommandPipelineTest, WrapAroundInterleavedDrainMatchesDirectProcessing) {
    const std::vector<OrderCommand> commands = {
        make_new_limit(1, Side::BUY, 10050, 100),
        make_new_limit(2, Side::SELL, 10055, 80),
        make_new_limit(3, Side::BUY, 10048, 50),
        make_cancel(1),
        make_new_limit(4, Side::BUY, 10060, 25),
    };

    MatchingEngine direct_engine;
    MatchingEngine pipeline_engine;
    SpscCommandPipeline pipeline(2);
    std::vector<EngineEvent> scratch;
    scratch.reserve(8);

    const auto direct_events = process_direct(direct_engine, commands, scratch);

    std::vector<EngineEvent> pipeline_events;
    for (const auto& command : commands) {
        while (!pipeline.try_enqueue(command)) {
            ASSERT_TRUE(pipeline.try_process_one(pipeline_engine, scratch, pipeline_events));
        }
    }
    pipeline.drain_all(pipeline_engine, scratch, pipeline_events);

    EXPECT_TRUE(events_equal(direct_events, pipeline_events));
    expect_books_equivalent(direct_engine, pipeline_engine);
    EXPECT_TRUE(pipeline.empty());
}

TEST(SpscCommandPipelineTest, BackpressureRejectsWhenQueueIsFull) {
    SpscCommandPipeline pipeline(2);

    EXPECT_TRUE(pipeline.try_enqueue(make_new_limit(1, Side::BUY, 10050, 10)));
    EXPECT_TRUE(pipeline.try_enqueue(make_new_limit(2, Side::SELL, 10055, 20)));
    EXPECT_TRUE(pipeline.full());
    EXPECT_FALSE(pipeline.try_enqueue(make_new_limit(3, Side::BUY, 10048, 5)));
}

TEST(SpscCommandPipelineTest, RunSequenceFailsWhenEnqueueWouldOverflow) {
    MatchingEngine engine;
    SpscCommandPipeline pipeline(1);
    std::vector<EngineEvent> scratch;
    std::vector<EngineEvent> events;

    const std::vector<OrderCommand> commands = {
        make_new_limit(1, Side::BUY, 10050, 100),
        make_new_limit(2, Side::SELL, 10055, 80),
    };

    EXPECT_FALSE(pipeline.run_sequence(engine, commands, scratch, events));
    EXPECT_TRUE(pipeline.full());
    EXPECT_FALSE(pipeline.empty());
}
