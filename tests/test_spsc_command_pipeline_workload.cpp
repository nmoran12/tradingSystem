#include "benchmarks/WorkloadGenerator.hpp"
#include "matching_engine/EngineEvent.hpp"
#include "matching_engine/MatchingEngine.hpp"
#include "matching_engine/OrderCommand.hpp"
#include "pipeline/SpscCommandPipeline.hpp"

#include <gtest/gtest.h>

#include <vector>

using namespace benchmarks;
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
    all_events.reserve(commands.size() * 2);
    for (const auto& command : commands) {
        engine.process_into(command, scratch);
        all_events.insert(all_events.end(), scratch.begin(), scratch.end());
    }
    return all_events;
}

std::vector<EngineEvent> process_via_run_sequence(SpscCommandPipeline& pipeline,
                                                  MatchingEngine& engine,
                                                  const std::vector<OrderCommand>& commands,
                                                  std::vector<EngineEvent>& scratch) {
    std::vector<EngineEvent> all_events;
    all_events.reserve(commands.size() * 2);
    if (!pipeline.run_sequence(engine, commands, scratch, all_events)) {
        ADD_FAILURE() << "pipeline.run_sequence failed";
        return all_events;
    }
    EXPECT_TRUE(pipeline.empty());
    return all_events;
}

std::vector<EngineEvent> process_via_interleaved_enqueue(SpscCommandPipeline& pipeline,
                                                         MatchingEngine& engine,
                                                         const std::vector<OrderCommand>& commands,
                                                         std::vector<EngineEvent>& scratch) {
    std::vector<EngineEvent> all_events;
    all_events.reserve(commands.size() * 2);
    for (const auto& command : commands) {
        while (!pipeline.try_enqueue(command)) {
            if (!pipeline.try_process_one(engine, scratch, all_events)) {
                ADD_FAILURE() << "expected drain when queue is full";
                return all_events;
            }
        }
    }
    pipeline.drain_all(engine, scratch, all_events);
    return all_events;
}

void expect_workload_equivalence(const std::vector<OrderCommand>& commands, size_t queue_capacity,
                                 bool use_interleaved_enqueue) {
    MatchingEngine direct_engine;
    MatchingEngine pipeline_engine;
    SpscCommandPipeline pipeline(queue_capacity);
    std::vector<EngineEvent> scratch;
    scratch.reserve(16);

    const auto direct_events = process_direct(direct_engine, commands, scratch);
    const auto pipeline_events =
        use_interleaved_enqueue
            ? process_via_interleaved_enqueue(pipeline, pipeline_engine, commands, scratch)
            : process_via_run_sequence(pipeline, pipeline_engine, commands, scratch);

    EXPECT_EQ(direct_events.size(), pipeline_events.size());
    EXPECT_TRUE(events_equal(direct_events, pipeline_events));
    expect_books_equivalent(direct_engine, pipeline_engine);
    EXPECT_FALSE(direct_engine.book().validate_invariants().has_value());
    EXPECT_TRUE(pipeline.empty());
}

WorkloadConfig make_workload_config(size_t command_count, uint64_t seed) {
    WorkloadConfig config;
    config.command_count = command_count;
    config.random_seed = seed;
    return config;
}

std::vector<OrderCommand> generate_workload(size_t command_count, uint64_t seed) {
    return WorkloadGenerator(make_workload_config(command_count, seed)).generate();
}

}  // namespace

TEST(SpscCommandPipelineWorkloadTest, Workload100CommandsSeed42EnqueueAllThenDrain) {
    const auto commands = generate_workload(100, 42);
    ASSERT_EQ(commands.size(), 100u);
    expect_workload_equivalence(commands, commands.size(), false);
}

TEST(SpscCommandPipelineWorkloadTest, Workload1000CommandsSeed42EnqueueAllThenDrain) {
    const auto commands = generate_workload(1000, 42);
    ASSERT_EQ(commands.size(), 1000u);
    expect_workload_equivalence(commands, commands.size(), false);
}

TEST(SpscCommandPipelineWorkloadTest, Workload100CommandsSeed42InterleavedSmallQueue) {
    const auto commands = generate_workload(100, 42);
    ASSERT_EQ(commands.size(), 100u);
    expect_workload_equivalence(commands, 4, true);
}

TEST(SpscCommandPipelineWorkloadTest, Workload1000CommandsSeed42InterleavedSmallQueue) {
    const auto commands = generate_workload(1000, 42);
    ASSERT_EQ(commands.size(), 1000u);
    expect_workload_equivalence(commands, 8, true);
}
