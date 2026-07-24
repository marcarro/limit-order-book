#pragma once

#include <vector>

#include "Order.h"
#include "PriceLevel.h"
#include "../common/FixedHashMap.h"
#include "../common/FixedPool.h"

namespace trading {

class Orderbook {
public:
    explicit Orderbook(OrderbookConfig config = {});

    Orderbook(const Orderbook&) = delete;
    Orderbook& operator=(const Orderbook&) = delete;
    Orderbook(Orderbook&&) = delete;
    Orderbook& operator=(Orderbook&&) = delete;

    OrderResult place_order(Order order, ExecutionBuffer& executions);
    OrderResult cancel_order(OrderId order_id);
    OrderResult modify_order(OrderId order_id, const Price& new_price, Quantity new_quantity, ExecutionBuffer& executions);

    std::vector<BookLevel> get_bid_levels(int depth = 10) const;
    std::vector<BookLevel> get_ask_levels(int depth = 10) const;
    Price get_mid_price() const;
    Price get_best_bid() const;
    Price get_best_ask() const;
    Quantity get_volume_at_price(const Price& price, Side side) const;

    std::size_t order_count() const { return order_by_id_.size(); }
    std::size_t price_level_count() const { return bid_levels_.size() + ask_levels_.size(); }
    std::size_t max_orders() const { return config_.max_orders; }
    void print_book() const;

private:
    OrderResult match(Order& taker, ExecutionBuffer& executions, PriceLevelSide& opposite);
    void add_resting_order(Order* order);
    void remove_resting_order(Order* order);
    bool is_valid(const Order& order) const;
    bool can_accept_resting_order(const Order& order) const;
    bool would_fully_match(const Order& order) const;
    PriceLevelSide& levels_for(Side side);
    const PriceLevelSide& levels_for(Side side) const;
    std::vector<BookLevel> levels_for_display(const PriceLevelSide& levels, bool descending, int depth) const;

    OrderbookConfig config_;
    memory::FixedPool<Order> order_pool_;
    memory::FixedPool<PriceLevel> price_level_pool_;
    PriceLevelSide bid_levels_;
    PriceLevelSide ask_levels_;
    memory::FixedHashMap<OrderId, Order*> order_by_id_;
};

} // namespace trading
