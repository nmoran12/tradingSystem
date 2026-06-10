#include "execution_v1_strategy.hpp"

#include <vector>

namespace arena = orderbook_arena::execution_v1;

std::vector<arena::Action> onBookUpdate(const arena::BookView& book,
                                        const arena::Portfolio& portfolio) {
    if (portfolio.remaining_quantity == 0) {
        return {};
    }

    if (book.events_remaining == 0) {
        std::vector<arena::Action> actions;
        for (const auto& order : portfolio.open_orders) {
            actions.push_back(arena::Action::cancel_order(order.order_id));
        }
        actions.push_back(arena::Action::market_order(
            1'000'000 + book.event_index, portfolio.remaining_quantity));
        return actions;
    }

    if (book.event_index == 0 && portfolio.open_orders.empty() &&
        !book.bids.empty()) {
        return {arena::Action::limit_order(
            1, book.bids.front().price_ticks + 1,
            portfolio.remaining_quantity)};
    }

    return {};
}

int main() {
    return arena::run_strategy_loop(onBookUpdate);
}
