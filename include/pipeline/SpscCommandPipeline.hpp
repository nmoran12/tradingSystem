#pragma once

#include "concurrency/SpscRingBuffer.hpp"
#include "matching_engine/EngineEvent.hpp"
#include "matching_engine/MatchingEngine.hpp"
#include "matching_engine/OrderCommand.hpp"

#include <cstddef>
#include <vector>

namespace pipeline {

/**
 * Single-threaded SPSC command ingest path in front of MatchingEngine.
 *
 * Commands are enqueued with try_enqueue, then drained in FIFO order via
 * MatchingEngine::process_into. This type does not spawn threads; it is a
 * deterministic simulation of producer/consumer roles on one thread.
 *
 * Direct process_into on the engine remains the correctness baseline.
 */
class SpscCommandPipeline {
public:
    explicit SpscCommandPipeline(size_t queue_capacity) : queue_(queue_capacity) {}

    bool try_enqueue(const matching_engine::OrderCommand& command) {
        return queue_.try_push(command);
    }

    bool try_enqueue(matching_engine::OrderCommand&& command) {
        return queue_.try_push(std::move(command));
    }

    /// Pop one command (if any), process it, append events to @p all_events.
    /// Returns false when the queue is empty.
    bool try_process_one(matching_engine::MatchingEngine& engine,
                         std::vector<matching_engine::EngineEvent>& scratch,
                         std::vector<matching_engine::EngineEvent>& all_events) {
        matching_engine::OrderCommand command;
        if (!queue_.try_pop(command)) {
            return false;
        }

        engine.process_into(command, scratch);
        all_events.insert(all_events.end(), scratch.begin(), scratch.end());
        return true;
    }

    /// Drain the queue completely in FIFO order.
    size_t drain_all(matching_engine::MatchingEngine& engine,
                     std::vector<matching_engine::EngineEvent>& scratch,
                     std::vector<matching_engine::EngineEvent>& all_events) {
        size_t processed = 0;
        while (try_process_one(engine, scratch, all_events)) {
            ++processed;
        }
        return processed;
    }

    /// Enqueue every command, then drain. Returns false if any enqueue fails.
    bool run_sequence(matching_engine::MatchingEngine& engine,
                      const std::vector<matching_engine::OrderCommand>& commands,
                      std::vector<matching_engine::EngineEvent>& scratch,
                      std::vector<matching_engine::EngineEvent>& all_events) {
        for (const auto& command : commands) {
            if (!try_enqueue(command)) {
                return false;
            }
        }
        drain_all(engine, scratch, all_events);
        return true;
    }

    [[nodiscard]] bool empty() const { return queue_.empty(); }
    [[nodiscard]] bool full() const { return queue_.full(); }
    [[nodiscard]] size_t queue_capacity() const { return queue_.capacity(); }

private:
    concurrency::SpscRingBuffer<matching_engine::OrderCommand> queue_;
};

}  // namespace pipeline
