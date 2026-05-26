#include "benchmarks/WorkloadGenerator.hpp"

#include <cmath>
#include <random>
#include <stdexcept>
#include <utility>
#include <vector>

namespace benchmarks {
namespace {

double normalized_mix(const WorkloadConfig& config) {
    return config.limit_order_pct + config.market_order_pct + config.cancel_pct +
           config.modify_pct;
}

}  // namespace

WorkloadGenerator::WorkloadGenerator(WorkloadConfig config) : config_(config) {
    if (config_.command_count == 0) {
        throw std::invalid_argument("command_count must be > 0");
    }
    const double mix = normalized_mix(config_);
    if (std::abs(mix - 1.0) > 1e-9) {
        throw std::invalid_argument("command mix percentages must sum to 1.0");
    }
    if (config_.min_quantity == 0 || config_.max_quantity < config_.min_quantity) {
        throw std::invalid_argument("invalid quantity range");
    }
    if (config_.symbols.empty()) {
        throw std::invalid_argument("at least one symbol is required");
    }
    for (const auto& symbol : config_.symbols) {
        if (symbol.empty() || symbol.size() > 16) {
            throw std::invalid_argument("symbols must be 1 to 16 bytes");
        }
    }
}

std::vector<matching_engine::OrderCommand> WorkloadGenerator::generate() const {
    std::mt19937_64 rng(config_.random_seed);
    std::uniform_real_distribution<double> kind_dist(0.0, 1.0);
    std::uniform_int_distribution<int64_t> price_offset_dist(-config_.price_half_range,
                                                             config_.price_half_range);
    std::uniform_int_distribution<uint64_t> quantity_dist(config_.min_quantity,
                                                         config_.max_quantity);
    std::uniform_int_distribution<size_t> symbol_index_dist(0, config_.symbols.size() - 1);

    std::vector<matching_engine::OrderCommand> commands;
    commands.reserve(config_.command_count);

    std::vector<uint64_t> active_order_ids;
    active_order_ids.reserve(config_.command_count / 2);
    uint64_t next_order_id = 1;

    for (size_t i = 0; i < config_.command_count; ++i) {
        const double roll = kind_dist(rng);
        const double limit_end = config_.limit_order_pct;
        const double market_end = limit_end + config_.market_order_pct;
        const double cancel_end = market_end + config_.cancel_pct;

        matching_engine::OrderCommand command;
        command.symbol = config_.symbols[symbol_index_dist(rng)];

        if (!active_order_ids.empty() && roll >= limit_end && roll < cancel_end) {
            std::uniform_int_distribution<size_t> active_index_dist(0, active_order_ids.size() - 1);
            const size_t index = active_index_dist(rng);
            command.type = matching_engine::OrderCommandType::CancelOrder;
            command.order_id = active_order_ids[index];
            command.side = market_data::Side::BUY;
            command.order_type = matching_engine::OrderType::Limit;
            command.price = 0;
            command.quantity = 0;
            commands.push_back(std::move(command));
            active_order_ids.erase(active_order_ids.begin() +
                                   static_cast<std::ptrdiff_t>(index));
            continue;
        }

        if (!active_order_ids.empty() && roll >= cancel_end) {
            std::uniform_int_distribution<size_t> active_index_dist(0, active_order_ids.size() - 1);
            const uint64_t target_id = active_order_ids[active_index_dist(rng)];
            command.type = matching_engine::OrderCommandType::ModifyOrder;
            command.order_id = target_id;
            command.side =
                (target_id % 2 == 0) ? market_data::Side::BUY : market_data::Side::SELL;
            command.order_type = matching_engine::OrderType::Limit;
            command.price =
                config_.price_midpoint + price_offset_dist(rng) + static_cast<int64_t>(i % 7);
            command.quantity = quantity_dist(rng);
            commands.push_back(std::move(command));
            continue;
        }

        command.type = matching_engine::OrderCommandType::NewOrder;
        command.order_id = next_order_id++;
        command.side = (command.order_id % 2 == 0) ? market_data::Side::BUY : market_data::Side::SELL;
        command.order_type = (roll < limit_end) ? matching_engine::OrderType::Limit
                                                : matching_engine::OrderType::Market;
        command.price = config_.price_midpoint + price_offset_dist(rng);
        if (command.order_type == matching_engine::OrderType::Market) {
            command.price = 0;
        } else if (command.price <= 0) {
            command.price = config_.price_midpoint;
        }
        command.quantity = quantity_dist(rng);
        const bool is_limit_order = command.order_type == matching_engine::OrderType::Limit;
        const uint64_t order_id = command.order_id;
        commands.push_back(std::move(command));
        if (is_limit_order) {
            active_order_ids.push_back(order_id);
        }
    }

    return commands;
}

}  // namespace benchmarks
