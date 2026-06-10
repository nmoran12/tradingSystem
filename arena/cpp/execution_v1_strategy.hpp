#pragma once

#include <cstdint>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace orderbook_arena::execution_v1 {

struct PriceLevel {
    std::int64_t price_ticks{};
    std::uint64_t quantity{};
};

struct OpenOrder {
    std::uint64_t order_id{};
    std::string side;
    std::string type;
    std::int64_t price_ticks{};
    std::uint64_t quantity{};
};

struct BookView {
    std::uint64_t episode_seed{};
    std::uint64_t event_index{};
    std::uint64_t events_remaining{};
    std::uint64_t timestamp_ms{};
    std::string symbol;
    std::vector<PriceLevel> bids;
    std::vector<PriceLevel> asks;
};

struct Portfolio {
    std::uint64_t target_quantity{};
    std::uint64_t filled_quantity{};
    std::uint64_t remaining_quantity{};
    std::uint64_t cash_spent_ticks{};
    std::optional<double> average_fill_price_ticks;
    std::vector<OpenOrder> open_orders;
};

enum class ActionType { MarketOrder, LimitOrder, CancelOrder };

struct Action {
    ActionType type{ActionType::MarketOrder};
    std::uint64_t order_id{};
    std::int64_t price_ticks{};
    std::uint64_t quantity{};

    static Action market_order(std::uint64_t order_id, std::uint64_t quantity) {
        return {ActionType::MarketOrder, order_id, 0, quantity};
    }

    static Action limit_order(std::uint64_t order_id, std::int64_t price_ticks,
                              std::uint64_t quantity) {
        return {ActionType::LimitOrder, order_id, price_ticks, quantity};
    }

    static Action cancel_order(std::uint64_t order_id) {
        return {ActionType::CancelOrder, order_id, 0, 0};
    }
};

using StrategyCallback =
    std::vector<Action> (*)(const BookView&, const Portfolio&);

namespace detail {

inline std::size_t find_key(std::string_view json, std::string_view key,
                            std::size_t start = 0) {
    const std::string token = "\"" + std::string(key) + "\"";
    const auto key_pos = json.find(token, start);
    if (key_pos == std::string_view::npos) {
        throw std::runtime_error("missing JSON field: " + std::string(key));
    }
    const auto colon_pos = json.find(':', key_pos + token.size());
    if (colon_pos == std::string_view::npos) {
        throw std::runtime_error("malformed JSON field: " + std::string(key));
    }
    return colon_pos + 1;
}

inline std::size_t skip_space(std::string_view json, std::size_t position) {
    while (position < json.size() &&
           (json[position] == ' ' || json[position] == '\t' ||
            json[position] == '\r' || json[position] == '\n')) {
        ++position;
    }
    return position;
}

inline std::uint64_t parse_u64(std::string_view json, std::string_view key,
                               std::size_t start = 0) {
    auto position = skip_space(json, find_key(json, key, start));
    if (position >= json.size() || json[position] < '0' || json[position] > '9') {
        throw std::runtime_error("field is not an unsigned integer: " +
                                 std::string(key));
    }
    std::uint64_t value = 0;
    while (position < json.size() && json[position] >= '0' &&
           json[position] <= '9') {
        value = value * 10 + static_cast<unsigned>(json[position] - '0');
        ++position;
    }
    return value;
}

inline std::int64_t parse_i64(std::string_view json, std::string_view key,
                              std::size_t start = 0) {
    auto position = skip_space(json, find_key(json, key, start));
    bool negative = false;
    if (position < json.size() && json[position] == '-') {
        negative = true;
        ++position;
    }
    if (position >= json.size() || json[position] < '0' || json[position] > '9') {
        throw std::runtime_error("field is not an integer: " + std::string(key));
    }
    std::int64_t value = 0;
    while (position < json.size() && json[position] >= '0' &&
           json[position] <= '9') {
        value = value * 10 + static_cast<unsigned>(json[position] - '0');
        ++position;
    }
    return negative ? -value : value;
}

inline std::string parse_string(std::string_view json, std::string_view key,
                                std::size_t start = 0) {
    auto position = skip_space(json, find_key(json, key, start));
    if (position >= json.size() || json[position] != '"') {
        throw std::runtime_error("field is not a string: " + std::string(key));
    }
    const auto end = json.find('"', position + 1);
    if (end == std::string_view::npos) {
        throw std::runtime_error("unterminated JSON string: " + std::string(key));
    }
    return std::string(json.substr(position + 1, end - position - 1));
}

inline std::string parse_top_level_string(std::string_view json,
                                          std::string_view key) {
    std::size_t depth = 0;
    for (std::size_t position = 0; position < json.size(); ++position) {
        if (json[position] == '{') {
            ++depth;
            continue;
        }
        if (json[position] == '}') {
            --depth;
            continue;
        }
        if (json[position] != '"') {
            continue;
        }

        const auto end = json.find('"', position + 1);
        if (end == std::string_view::npos) {
            throw std::runtime_error("unterminated top-level JSON string");
        }
        if (depth == 1 && json.substr(position + 1, end - position - 1) == key) {
            auto value_position = skip_space(json, end + 1);
            if (value_position >= json.size() || json[value_position] != ':') {
                throw std::runtime_error("malformed top-level JSON field: " +
                                         std::string(key));
            }
            value_position = skip_space(json, value_position + 1);
            if (value_position >= json.size() || json[value_position] != '"') {
                throw std::runtime_error("top-level field is not a string: " +
                                         std::string(key));
            }
            const auto value_end = json.find('"', value_position + 1);
            if (value_end == std::string_view::npos) {
                throw std::runtime_error("unterminated top-level JSON value: " +
                                         std::string(key));
            }
            return std::string(
                json.substr(value_position + 1, value_end - value_position - 1));
        }
        position = end;
    }
    throw std::runtime_error("missing top-level JSON field: " + std::string(key));
}

inline std::optional<double> parse_optional_double(
    std::string_view json, std::string_view key, std::size_t start = 0) {
    auto position = skip_space(json, find_key(json, key, start));
    if (json.substr(position, 4) == "null") {
        return std::nullopt;
    }
    std::size_t consumed = 0;
    const auto value = std::stod(std::string(json.substr(position)), &consumed);
    if (consumed == 0) {
        throw std::runtime_error("field is not a number or null: " +
                                 std::string(key));
    }
    return value;
}

inline std::pair<std::size_t, std::size_t> object_range(
    std::string_view json, std::string_view key, std::size_t start = 0) {
    auto begin = skip_space(json, find_key(json, key, start));
    if (begin >= json.size() || json[begin] != '{') {
        throw std::runtime_error("field is not an object: " + std::string(key));
    }
    std::size_t depth = 0;
    for (std::size_t position = begin; position < json.size(); ++position) {
        if (json[position] == '{') {
            ++depth;
        } else if (json[position] == '}' && --depth == 0) {
            return {begin, position + 1};
        }
    }
    throw std::runtime_error("unterminated JSON object: " + std::string(key));
}

inline std::pair<std::size_t, std::size_t> array_range(
    std::string_view json, std::string_view key, std::size_t start = 0) {
    auto begin = skip_space(json, find_key(json, key, start));
    if (begin >= json.size() || json[begin] != '[') {
        throw std::runtime_error("field is not an array: " + std::string(key));
    }
    std::size_t depth = 0;
    for (std::size_t position = begin; position < json.size(); ++position) {
        if (json[position] == '[') {
            ++depth;
        } else if (json[position] == ']' && --depth == 0) {
            return {begin, position + 1};
        }
    }
    throw std::runtime_error("unterminated JSON array: " + std::string(key));
}

inline std::vector<std::string_view> array_objects(std::string_view json,
                                                   std::string_view key,
                                                   std::size_t start = 0) {
    const auto [array_begin, array_end] = array_range(json, key, start);
    std::vector<std::string_view> objects;
    std::size_t position = array_begin + 1;
    while (position < array_end - 1) {
        position = json.find('{', position);
        if (position == std::string_view::npos || position >= array_end) {
            break;
        }
        std::size_t depth = 0;
        for (std::size_t end = position; end < array_end; ++end) {
            if (json[end] == '{') {
                ++depth;
            } else if (json[end] == '}' && --depth == 0) {
                objects.push_back(json.substr(position, end - position + 1));
                position = end + 1;
                break;
            }
        }
    }
    return objects;
}

inline std::vector<PriceLevel> parse_levels(std::string_view json,
                                            std::string_view side) {
    std::vector<PriceLevel> levels;
    for (const auto object : array_objects(json, side)) {
        levels.push_back(
            {parse_i64(object, "price_ticks"), parse_u64(object, "quantity")});
    }
    return levels;
}

inline std::vector<OpenOrder> parse_open_orders(std::string_view json) {
    std::vector<OpenOrder> orders;
    for (const auto object : array_objects(json, "open_orders")) {
        orders.push_back(
            {parse_u64(object, "order_id"),
             parse_string(object, "side"),
             parse_string(object, "type"),
             parse_i64(object, "price_ticks"),
             parse_u64(object, "quantity")});
    }
    return orders;
}

inline BookView parse_book_view(std::string_view line) {
    const auto [book_begin, book_end] = object_range(line, "book");
    const auto book_json = line.substr(book_begin, book_end - book_begin);
    return {
        parse_u64(line, "episode_seed"),
        parse_u64(line, "event_index"),
        parse_u64(line, "events_remaining"),
        parse_u64(line, "timestamp_ms"),
        parse_string(line, "symbol"),
        parse_levels(book_json, "bids"),
        parse_levels(book_json, "asks"),
    };
}

inline Portfolio parse_portfolio(std::string_view line) {
    const auto [begin, end] = object_range(line, "portfolio");
    const auto portfolio_json = line.substr(begin, end - begin);
    return {
        parse_u64(portfolio_json, "target_quantity"),
        parse_u64(portfolio_json, "filled_quantity"),
        parse_u64(portfolio_json, "remaining_quantity"),
        parse_u64(portfolio_json, "cash_spent_ticks"),
        parse_optional_double(portfolio_json, "average_fill_price_ticks"),
        parse_open_orders(portfolio_json),
    };
}

inline void write_actions(const std::vector<Action>& actions) {
    std::cout << "{\"type\":\"actions\",\"actions\":[";
    for (std::size_t index = 0; index < actions.size(); ++index) {
        if (index != 0) {
            std::cout << ',';
        }
        const auto& action = actions[index];
        switch (action.type) {
            case ActionType::MarketOrder:
                std::cout << "{\"type\":\"market_order\",\"order_id\":"
                          << action.order_id << ",\"quantity\":"
                          << action.quantity << '}';
                break;
            case ActionType::LimitOrder:
                std::cout << "{\"type\":\"limit_order\",\"order_id\":"
                          << action.order_id << ",\"price_ticks\":"
                          << action.price_ticks << ",\"quantity\":"
                          << action.quantity << '}';
                break;
            case ActionType::CancelOrder:
                std::cout << "{\"type\":\"cancel_order\",\"order_id\":"
                          << action.order_id << '}';
                break;
        }
    }
    std::cout << "]}" << std::endl;
}

}  // namespace detail

inline int run_strategy_loop(StrategyCallback callback) {
    std::string line;
    try {
        while (std::getline(std::cin, line)) {
            const auto message_type =
                detail::parse_top_level_string(line, "type");
            if (message_type == "book_update") {
                const auto book = detail::parse_book_view(line);
                const auto portfolio = detail::parse_portfolio(line);
                detail::write_actions(callback(book, portfolio));
            } else if (message_type == "episode_end") {
                continue;
            } else if (message_type == "evaluation_end") {
                return 0;
            } else {
                std::cerr << "unsupported protocol message: " << message_type
                          << '\n';
                return 2;
            }
        }
    } catch (const std::exception& error) {
        std::cerr << "protocol error: " << error.what() << '\n';
        return 3;
    }
    std::cerr << "protocol ended before evaluation_end\n";
    return 4;
}

}  // namespace orderbook_arena::execution_v1
