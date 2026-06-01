#include "concurrency/SpscRingBuffer.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <stdexcept>
#include <thread>
#include <vector>

using concurrency::SpscRingBuffer;

namespace {

struct MoveOnlyValue {
    MoveOnlyValue() = default;
    explicit MoveOnlyValue(int input) : value(std::make_unique<int>(input)) {}

    MoveOnlyValue(const MoveOnlyValue&) = delete;
    MoveOnlyValue& operator=(const MoveOnlyValue&) = delete;
    MoveOnlyValue(MoveOnlyValue&&) noexcept = default;
    MoveOnlyValue& operator=(MoveOnlyValue&&) noexcept = default;

    std::unique_ptr<int> value;
};

}  // namespace

TEST(SpscRingBufferTest, EmptyBufferPopFails) {
    SpscRingBuffer<int> buffer(4);
    int value = 0;

    EXPECT_TRUE(buffer.empty());
    EXPECT_FALSE(buffer.full());
    EXPECT_FALSE(buffer.try_pop(value));
}

TEST(SpscRingBufferTest, ZeroCapacityIsRejected) {
    EXPECT_THROW(SpscRingBuffer<int> buffer(0), std::invalid_argument);
}

TEST(SpscRingBufferTest, CapacityOneStoresOneElement) {
    SpscRingBuffer<int> buffer(1);

    EXPECT_EQ(buffer.capacity(), 1u);
    EXPECT_TRUE(buffer.try_push(11));
    EXPECT_TRUE(buffer.full());
    EXPECT_FALSE(buffer.try_push(12));

    int value = 0;
    ASSERT_TRUE(buffer.try_pop(value));
    EXPECT_EQ(value, 11);
    EXPECT_TRUE(buffer.empty());
}

TEST(SpscRingBufferTest, PushThenPopPreservesValue) {
    SpscRingBuffer<int> buffer(2);

    ASSERT_TRUE(buffer.try_push(42));

    int value = 0;
    ASSERT_TRUE(buffer.try_pop(value));
    EXPECT_EQ(value, 42);
    EXPECT_TRUE(buffer.empty());
}

TEST(SpscRingBufferTest, PreservesFifoOrdering) {
    SpscRingBuffer<int> buffer(4);

    ASSERT_TRUE(buffer.try_push(1));
    ASSERT_TRUE(buffer.try_push(2));
    ASSERT_TRUE(buffer.try_push(3));

    int value = 0;
    ASSERT_TRUE(buffer.try_pop(value));
    EXPECT_EQ(value, 1);
    ASSERT_TRUE(buffer.try_pop(value));
    EXPECT_EQ(value, 2);
    ASSERT_TRUE(buffer.try_pop(value));
    EXPECT_EQ(value, 3);
}

TEST(SpscRingBufferTest, FullBufferRejectsExtraPush) {
    SpscRingBuffer<int> buffer(2);

    EXPECT_EQ(buffer.capacity(), 2u);
    ASSERT_TRUE(buffer.try_push(10));
    ASSERT_TRUE(buffer.try_push(20));

    EXPECT_TRUE(buffer.full());
    EXPECT_FALSE(buffer.try_push(30));

    int value = 0;
    ASSERT_TRUE(buffer.try_pop(value));
    EXPECT_EQ(value, 10);
    ASSERT_TRUE(buffer.try_pop(value));
    EXPECT_EQ(value, 20);
}

TEST(SpscRingBufferTest, WrapAroundPreservesOrder) {
    SpscRingBuffer<int> buffer(3);
    int value = 0;

    ASSERT_TRUE(buffer.try_push(1));
    ASSERT_TRUE(buffer.try_push(2));
    ASSERT_TRUE(buffer.try_push(3));
    ASSERT_TRUE(buffer.try_pop(value));
    EXPECT_EQ(value, 1);
    ASSERT_TRUE(buffer.try_pop(value));
    EXPECT_EQ(value, 2);

    ASSERT_TRUE(buffer.try_push(4));
    ASSERT_TRUE(buffer.try_push(5));

    ASSERT_TRUE(buffer.try_pop(value));
    EXPECT_EQ(value, 3);
    ASSERT_TRUE(buffer.try_pop(value));
    EXPECT_EQ(value, 4);
    ASSERT_TRUE(buffer.try_pop(value));
    EXPECT_EQ(value, 5);
    EXPECT_TRUE(buffer.empty());
}

TEST(SpscRingBufferTest, SupportsMoveOnlyTypes) {
    SpscRingBuffer<MoveOnlyValue> buffer(2);

    ASSERT_TRUE(buffer.try_push(MoveOnlyValue(7)));

    MoveOnlyValue output;
    ASSERT_TRUE(buffer.try_pop(output));
    ASSERT_NE(output.value, nullptr);
    EXPECT_EQ(*output.value, 7);
}

TEST(SpscRingBufferTest, RepeatedPushPopCycles) {
    SpscRingBuffer<int> buffer(5);

    for (int i = 0; i < 1000; ++i) {
        ASSERT_TRUE(buffer.try_push(i));
        int value = -1;
        ASSERT_TRUE(buffer.try_pop(value));
        EXPECT_EQ(value, i);
    }

    EXPECT_TRUE(buffer.empty());
}

TEST(SpscRingBufferTest, DeterministicTwoThreadSmokeTest) {
    constexpr int item_count = 10000;
    SpscRingBuffer<int> buffer(64);
    std::vector<int> consumed;
    consumed.reserve(item_count);

    std::thread producer([&buffer] {
        for (int value = 0; value < item_count; ++value) {
            while (!buffer.try_push(value)) {
                std::this_thread::yield();
            }
        }
    });

    std::thread consumer([&buffer, &consumed] {
        while (static_cast<int>(consumed.size()) < item_count) {
            int value = -1;
            if (buffer.try_pop(value)) {
                consumed.push_back(value);
            } else {
                std::this_thread::yield();
            }
        }
    });

    producer.join();
    consumer.join();

    ASSERT_EQ(consumed.size(), static_cast<size_t>(item_count));
    for (int expected = 0; expected < item_count; ++expected) {
        EXPECT_EQ(consumed[static_cast<size_t>(expected)], expected);
    }
    EXPECT_TRUE(buffer.empty());
}
