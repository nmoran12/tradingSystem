#include "metrics/LatencyTracker.hpp"

#include <gtest/gtest.h>

using namespace metrics;

TEST(LatencyTrackerTest, RecordsDurations) {
    LatencyTracker tracker;
    tracker.record(100);
    tracker.record(200);
    EXPECT_EQ(tracker.count(), 2u);
    EXPECT_EQ(tracker.min(), 100u);
    EXPECT_EQ(tracker.max(), 200u);
}

TEST(LatencyTrackerTest, CalculatesAverage) {
    LatencyTracker tracker;
    tracker.record(100);
    tracker.record(300);
    EXPECT_DOUBLE_EQ(tracker.average(), 200.0);
}

TEST(LatencyTrackerTest, CalculatesPercentilesCorrectly) {
    LatencyTracker tracker;
    for (uint64_t value = 1; value <= 100; ++value) {
        tracker.record(value);
    }

    EXPECT_EQ(tracker.p50(), 50u);
    EXPECT_EQ(tracker.p95(), 95u);
    EXPECT_EQ(tracker.p99(), 99u);
}

TEST(LatencyTrackerTest, ResetClearsSamples) {
    LatencyTracker tracker;
    tracker.record(100);
    tracker.reset();
    EXPECT_EQ(tracker.count(), 0u);
    EXPECT_DOUBLE_EQ(tracker.average(), 0.0);
}
