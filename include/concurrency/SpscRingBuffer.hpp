#pragma once

#include <atomic>
#include <cstddef>
#include <memory>
#include <stdexcept>
#include <utility>

namespace concurrency {

/**
 * Fixed-capacity single-producer / single-consumer ring buffer.
 *
 * The constructor argument is the exact usable capacity reported by capacity().
 * Internally the storage is rounded up to a power of two so indices can use a
 * mask. The buffer does not leave an unusable slot empty; fullness is tracked by
 * monotonic read/write counters instead.
 *
 * Threading contract:
 * - one producer thread calls try_push
 * - one consumer thread calls try_pop
 * - empty/full/capacity are observational helpers and may be called by either
 *   thread
 * - no dynamic allocation occurs after construction
 */
template <typename T>
class SpscRingBuffer {
public:
    explicit SpscRingBuffer(size_t capacity)
        : capacity_(validate_capacity(capacity)),
          storage_capacity_(next_power_of_two(capacity)),
          mask_(storage_capacity_ - 1),
          storage_(std::allocator_traits<Allocator>::allocate(allocator_, storage_capacity_)) {}

    ~SpscRingBuffer() {
        destroy_remaining();
        std::allocator_traits<Allocator>::deallocate(allocator_, storage_, storage_capacity_);
    }

    SpscRingBuffer(const SpscRingBuffer&) = delete;
    SpscRingBuffer& operator=(const SpscRingBuffer&) = delete;
    SpscRingBuffer(SpscRingBuffer&&) = delete;
    SpscRingBuffer& operator=(SpscRingBuffer&&) = delete;

    bool try_push(const T& value) {
        return emplace(value);
    }

    bool try_push(T&& value) {
        return emplace(std::move(value));
    }

    bool try_pop(T& out) {
        const size_t read_index = read_.value.load(std::memory_order_relaxed);
        const size_t write_index = write_.value.load(std::memory_order_acquire);
        if (read_index == write_index) {
            return false;
        }

        T* item = slot(read_index);
        out = std::move(*item);
        std::allocator_traits<Allocator>::destroy(allocator_, item);
        read_.value.store(read_index + 1, std::memory_order_release);
        return true;
    }

    bool empty() const {
        return read_.value.load(std::memory_order_acquire) ==
               write_.value.load(std::memory_order_acquire);
    }

    bool full() const {
        const size_t write_index = write_.value.load(std::memory_order_acquire);
        const size_t read_index = read_.value.load(std::memory_order_acquire);
        return write_index - read_index >= capacity_;
    }

    size_t capacity() const {
        return capacity_;
    }

private:
    using Allocator = std::allocator<T>;

    struct alignas(64) PaddedAtomicSize {
        std::atomic<size_t> value{0};
    };

    static size_t validate_capacity(size_t capacity) {
        if (capacity == 0) {
            throw std::invalid_argument("SpscRingBuffer capacity must be greater than zero");
        }
        return capacity;
    }

    static size_t next_power_of_two(size_t value) {
        size_t result = 1;
        while (result < value) {
            result <<= 1;
        }
        return result;
    }

    template <typename U>
    bool emplace(U&& value) {
        const size_t write_index = write_.value.load(std::memory_order_relaxed);
        const size_t read_index = read_.value.load(std::memory_order_acquire);
        if (write_index - read_index >= capacity_) {
            return false;
        }

        std::allocator_traits<Allocator>::construct(allocator_, slot(write_index),
                                                    std::forward<U>(value));
        write_.value.store(write_index + 1, std::memory_order_release);
        return true;
    }

    T* slot(size_t index) {
        return storage_ + (index & mask_);
    }

    const T* slot(size_t index) const {
        return storage_ + (index & mask_);
    }

    void destroy_remaining() {
        size_t read_index = read_.value.load(std::memory_order_relaxed);
        const size_t write_index = write_.value.load(std::memory_order_relaxed);
        while (read_index != write_index) {
            std::allocator_traits<Allocator>::destroy(allocator_, slot(read_index));
            ++read_index;
        }
    }

    const size_t capacity_;
    const size_t storage_capacity_;
    const size_t mask_;
    Allocator allocator_;
    T* storage_;
    PaddedAtomicSize read_;
    PaddedAtomicSize write_;
};

}  // namespace concurrency
