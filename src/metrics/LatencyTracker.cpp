#include "metrics/LatencyTracker.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <stdexcept>

namespace metrics {

void LatencyTracker::record(uint64_t duration_ns) { samples_.push_back(duration_ns); }

void LatencyTracker::reset() { samples_.clear(); }

size_t LatencyTracker::count() const { return samples_.size(); }

uint64_t LatencyTracker::min() const {
    if (samples_.empty()) {
        return 0;
    }
    return *std::min_element(samples_.begin(), samples_.end());
}

uint64_t LatencyTracker::max() const {
    if (samples_.empty()) {
        return 0;
    }
    return *std::max_element(samples_.begin(), samples_.end());
}

double LatencyTracker::average() const {
    if (samples_.empty()) {
        return 0.0;
    }
    const auto sum = std::accumulate(samples_.begin(), samples_.end(), 0ULL);
    return static_cast<double>(sum) / static_cast<double>(samples_.size());
}

uint64_t LatencyTracker::percentile(double p) const {
    if (samples_.empty()) {
        return 0;
    }
    if (p < 0.0 || p > 100.0) {
        throw std::invalid_argument("percentile must be in [0, 100]");
    }

    std::vector<uint64_t> sorted = samples_;
    std::sort(sorted.begin(), sorted.end());

    // Nearest-rank method: index = ceil(p/100 * n) - 1
    const auto rank = static_cast<size_t>(std::ceil((p / 100.0) * static_cast<double>(sorted.size())));
    const size_t index = std::min(rank, sorted.size()) - 1;
    return sorted[index];
}

}  // namespace metrics
