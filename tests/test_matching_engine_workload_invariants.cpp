#include "benchmarks/WorkloadGenerator.hpp"
#include "matching_engine/EngineEvent.hpp"
#include "matching_engine/MatchingEngine.hpp"

#include <gtest/gtest.h>

#include <vector>

using namespace benchmarks;
using namespace matching_engine;

namespace {

uint64_t count_trades(const std::vector<EngineEvent>& events) {
    uint64_t trades = 0;
    for (const auto& event : events) {
        if (event.type == EngineEventType::Trade && event.trade) {
            ++trades;
        }
    }
    return trades;
}

}  // namespace

TEST(MatchingEngineWorkloadInvariantTest, TwoThousandCommandWorkloadMaintainsInvariants) {
    WorkloadConfig config;
    config.command_count = 2'000;
    config.random_seed = 42;
    const auto commands = WorkloadGenerator(config).generate();
    ASSERT_EQ(commands.size(), 2'000u);

    MatchingEngine engine;
    engine.reserve_book_capacity(config.command_count / 2);
    std::vector<EngineEvent> scratch;
    scratch.reserve(16);

    uint64_t total_trades = 0;
    for (const auto& command : commands) {
        engine.process_into(command, scratch);
        total_trades += count_trades(scratch);
        ASSERT_FALSE(engine.book().validate_invariants().has_value())
            << "invariant failure after order_id=" << command.order_id;
    }

    EXPECT_GT(total_trades, 0u);
    EXPECT_GT(engine.book().active_order_count(), 0u);
    EXPECT_FALSE(engine.book().validate_invariants().has_value());
}
