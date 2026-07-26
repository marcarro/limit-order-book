#include "orderbook/Orderbook.h"

#include <algorithm>
#include <cassert>
#include <iomanip>
#include <iostream>
#include <stdexcept>

namespace trading {

Orderbook::Orderbook(OrderbookConfig config)
    : config_(config),
      order_pool_(config.max_orders),
      price_level_pool_(config.max_price_levels),
      bid_levels_(true, config.max_price_levels),
      ask_levels_(false, config.max_price_levels),
      order_by_id_(config.max_orders) {
    if (config.max_orders == 0 || config.max_price_levels == 0) {
        throw std::invalid_argument("orderbook capacities must be positive");
    }
}

OrderResult Orderbook::place_order(Order order, ExecutionBuffer& executions) {
    if (!is_valid(order)) {
        return OrderResult::INVALID_ORDER;
    }
    if (order_by_id_.find(order.order_id)) {
        return OrderResult::DUPLICATE_ORDER_ID;
    }
    if (executions.capacity() < config_.max_orders) {
        return OrderResult::EXECUTION_BUFFER_TOO_SMALL;
    }
    if (!can_accept_resting_order(order) && !would_fully_match(order)) {
        const PriceLevelSide& levels = levels_for(order.side);
        return (order_pool_.full() || order_by_id_.full())
            ? OrderResult::BOOK_FULL
            : (levels.full() || price_level_pool_.full() ? OrderResult::PRICE_LEVEL_LIMIT : OrderResult::BOOK_FULL);
    }

    executions.clear();
    order.next = nullptr;
    order.prev = nullptr;
    order.level = nullptr;

    const Quantity initial_quantity = order.quantity;
    const OrderResult match_result = match(order, executions, levels_for(order.side == Side::BUY ? Side::SELL : Side::BUY));
    if (order.quantity == 0) {
        return OrderResult::COMPLETE_FILL;
    }

    Order* resting = order_pool_.acquire();
    assert(resting);
    *resting = order;
    add_resting_order(resting);
    const bool inserted = order_by_id_.insert(resting->order_id, resting);
    assert(inserted);
    (void)inserted;

    return order.quantity == initial_quantity ? OrderResult::SUCCESS : match_result;
}

OrderResult Orderbook::cancel_order(OrderId order_id) {
    Order* const* order = order_by_id_.find(order_id);
    if (!order) {
        return OrderResult::ORDER_NOT_FOUND;
    }
    remove_resting_order(*order);
    return OrderResult::SUCCESS;
}

OrderResult Orderbook::modify_order(
    OrderId order_id,
    const Price& new_price,
    Quantity new_quantity,
    ExecutionBuffer& executions
) {
    Order* const* found = order_by_id_.find(order_id);
    if (!found) {
        return OrderResult::ORDER_NOT_FOUND;
    }
    if (new_price.raw_value() <= 0 || new_quantity <= 0) {
        return OrderResult::INVALID_ORDER;
    }
    if (executions.capacity() < config_.max_orders) {
        return OrderResult::EXECUTION_BUFFER_TOO_SMALL;
    }

    Order* existing = *found;
    if (existing->price == new_price && new_quantity < existing->quantity) {
        executions.clear();
        const Quantity old_quantity = existing->quantity;
        existing->quantity = new_quantity;
        existing->level->update_quantity(existing, old_quantity);
        return OrderResult::SUCCESS;
    }

    PriceLevelSide& side = levels_for(existing->side);
    const bool needs_new_level = side.find(new_price) == nullptr;
    const bool frees_old_level = existing->level->order_count == 1;
    if (needs_new_level && !frees_old_level && (side.full() || price_level_pool_.full())) {
        return OrderResult::PRICE_LEVEL_LIMIT;
    }

    Order replacement = *existing;
    replacement.price = new_price;
    replacement.quantity = new_quantity;
    replacement.next = nullptr;
    replacement.prev = nullptr;
    replacement.level = nullptr;
    remove_resting_order(existing);
    return place_order(replacement, executions);
}

std::vector<BookLevel> Orderbook::get_bid_levels(int depth) const {
    return levels_for_display(bid_levels_, true, depth);
}

std::vector<BookLevel> Orderbook::get_ask_levels(int depth) const {
    return levels_for_display(ask_levels_, false, depth);
}

Price Orderbook::get_mid_price() const {
    const Price bid = get_best_bid();
    const Price ask = get_best_ask();
    return bid.raw_value() == 0 || ask.raw_value() == 0 ? Price{} : (bid + ask) / 2;
}

Price Orderbook::get_best_bid() const {
    const PriceLevel* level = bid_levels_.best();
    return level ? level->price : Price{};
}

Price Orderbook::get_best_ask() const {
    const PriceLevel* level = ask_levels_.best();
    return level ? level->price : Price{};
}

Quantity Orderbook::get_volume_at_price(const Price& price, Side side) const {
    const PriceLevel* level = levels_for(side).find(price);
    return level ? level->total_volume : 0;
}

void Orderbook::print_book() const {
    std::cout << "--------- ORDER BOOK ---------\nASKS:\n";
    for (const BookLevel& level : get_ask_levels()) {
        std::cout << std::setw(10) << level.price.to_string() << " | "
                  << std::setw(10) << level.total_volume << " | "
                  << std::setw(5) << level.order_count << '\n';
    }
    std::cout << "BIDS:\n";
    for (const BookLevel& level : get_bid_levels()) {
        std::cout << std::setw(10) << level.price.to_string() << " | "
                  << std::setw(10) << level.total_volume << " | "
                  << std::setw(5) << level.order_count << '\n';
    }
}

OrderResult Orderbook::match(Order& taker, ExecutionBuffer& executions, PriceLevelSide& opposite) {
    bool matched = false;
    while (taker.quantity > 0 && !opposite.empty()) {
        PriceLevel* level = opposite.best();
        const bool price_does_not_cross = taker.side == Side::BUY
            ? level->price > taker.price
            : level->price < taker.price;
        if (price_does_not_cross) {
            break;
        }

        while (taker.quantity > 0 && level->head) {
            Order* maker = level->head;
            const Quantity traded = std::min(taker.quantity, maker->quantity);
            executions.push_unchecked(Execution{
                taker.order_id,
                maker->order_id,
                taker.client_id,
                maker->client_id,
                maker->price,
                traded,
                taker.side,
            });
            taker.quantity -= traded;
            maker->quantity -= traded;
            level->total_volume -= traded;
            matched = true;
            if (maker->quantity == 0) {
                remove_resting_order(maker);
            }
        }
    }
    return matched ? OrderResult::PARTIAL_FILL : OrderResult::SUCCESS;
}

void Orderbook::add_resting_order(Order* order) {
    PriceLevelSide& levels = levels_for(order->side);
    PriceLevel* level = levels.find(order->price);
    if (!level) {
        level = levels.create(order->price, price_level_pool_);
        assert(level);
    }
    level->add_order(order);
}

void Orderbook::remove_resting_order(Order* order) {
    PriceLevel* level = order->level;
    assert(level);
    PriceLevelSide& levels = levels_for(order->side);
    level->remove_order(order);
    const bool erased = order_by_id_.erase(order->order_id);
    assert(erased);
    (void)erased;
    order_pool_.release(order);
    if (level->order_count == 0) {
        levels.remove(level, price_level_pool_);
    }
}

bool Orderbook::is_valid(const Order& order) const {
    return order.order_id != 0 && order.client_id != 0 && order.quantity > 0 && order.price.raw_value() > 0;
}

bool Orderbook::can_accept_resting_order(const Order& order) const {
    if (order_pool_.full() || order_by_id_.full()) {
        return false;
    }
    const PriceLevelSide& levels = levels_for(order.side);
    return levels.find(order.price) || (!levels.full() && !price_level_pool_.full());
}

bool Orderbook::would_fully_match(const Order& order) const {
    Quantity remaining = order.quantity;
    const PriceLevelSide& opposite = levels_for(order.side == Side::BUY ? Side::SELL : Side::BUY);
    opposite.for_each([&](const PriceLevel& level) {
        if (remaining == 0) {
            return;
        }
        const bool crosses = order.side == Side::BUY ? level.price <= order.price : level.price >= order.price;
        if (crosses) {
            remaining -= std::min(remaining, level.total_volume);
        }
    });
    return remaining == 0;
}

PriceLevelSide& Orderbook::levels_for(Side side) {
    return side == Side::BUY ? bid_levels_ : ask_levels_;
}

const PriceLevelSide& Orderbook::levels_for(Side side) const {
    return side == Side::BUY ? bid_levels_ : ask_levels_;
}

std::vector<BookLevel> Orderbook::levels_for_display(const PriceLevelSide& levels, bool descending, int depth) const {
    if (depth <= 0) {
        return {};
    }
    std::vector<const PriceLevel*> price_levels;
    levels.collect(price_levels);
    std::sort(price_levels.begin(), price_levels.end(), [descending](const PriceLevel* lhs, const PriceLevel* rhs) {
        return descending ? lhs->price > rhs->price : lhs->price < rhs->price;
    });

    const std::size_t count = std::min<std::size_t>(price_levels.size(), static_cast<std::size_t>(depth));
    std::vector<BookLevel> result;
    result.reserve(count);
    for (std::size_t index = 0; index < count; ++index) {
        const PriceLevel* level = price_levels[index];
        result.push_back(BookLevel{level->price, level->total_volume, level->order_count});
    }
    return result;
}

} // namespace trading
