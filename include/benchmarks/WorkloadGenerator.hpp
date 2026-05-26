#pragma once

#include "matching_engine/OrderCommand.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace benchmarks {

struct WorkloadConfig {
    size_t command_count{1'000'000};
    double limit_order_pct{0.70};
    double market_order_pct{0.10};
    double cancel_pct{0.10};
    double modify_pct{0.10};
    int64_t price_midpoint{10'000};
    int64_t price_half_range{500};
    uint64_t min_quantity{1};
    uint64_t max_quantity{1'000};
    uint64_t random_seed{42};
    std::vector<std::string> symbols{"AAPL", "MSFT", "NVDA"};
};

class WorkloadGenerator {
public:
    explicit WorkloadGenerator(WorkloadConfig config);

    [[nodiscard]] std::vector<matching_engine::OrderCommand> generate() const;

private:
    WorkloadConfig config_;
};

}  // namespace benchmarks
