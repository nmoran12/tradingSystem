#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace metrics {

class LatencyTracker {
public:
    void record(uint64_t duration_ns);
    void reset();

    [[nodiscard]] size_t count() const;
    [[nodiscard]] uint64_t min() const;
    [[nodiscard]] uint64_t max() const;
    [[nodiscard]] double average() const;
    [[nodiscard]] uint64_t percentile(double p) const;

    [[nodiscard]] uint64_t p50() const { return percentile(50.0); }
    [[nodiscard]] uint64_t p95() const { return percentile(95.0); }
    [[nodiscard]] uint64_t p99() const { return percentile(99.0); }

private:
    std::vector<uint64_t> samples_;
};

}  // namespace metrics
