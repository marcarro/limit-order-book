#ifndef ORDERBOOK_IMPL_H
#define ORDERBOOK_IMPL_H
#pragma once

#include <list>
#include <map>
#include <utility>
#include <functional>
#include "Order.h"
#include "Orderbook.h"

// Custom comparators for Price type
struct PriceGreater {
    bool operator()(const Price& a, const Price& b) const {
        return a > b;
    }
};

struct PriceLess {
    bool operator()(const Price& a, const Price& b) const {
        return a < b;
    }
};

template <typename PriceComparator>
struct BookView {
	std::map<Price, std::list<Order>, PriceComparator>& book;
	std::map<Price, int, PriceComparator>& volume_tracker;
	std::function<bool(const Price&, const Price&)> price_matches_condition;
};

class OrderbookImpl {
public:
	Orderbook::Result place_order(const Order& order, std::vector<Orderbook::TradeInfo>& trades);
    Orderbook::Result cancel_order(int order_id);
    Orderbook::Result modify_order(int order_id, const Price& new_price, int new_volume);

	// Compatibility method for transition
    Orderbook::Result modify_order(int order_id, double new_price, int new_volume) {
        return modify_order(order_id, Price(new_price), new_volume);
    }

	std::vector<Orderbook::BookLevel> get_bid_levels(int depth) const;
    std::vector<Orderbook::BookLevel> get_ask_levels(int depth) const;
    Price get_mid_price() const;
    Price get_best_bid() const;
    Price get_best_ask() const;
    int get_volume_at_price(const Price& price, buy_or_sell side) const;

    int get_volume_at_price(double price, buy_or_sell side) const {
		return get_volume_at_price(Price(price), side);
	}
    
    void print_book() const;
private:
    std::map<Price, std::list<Order>, PriceGreater> bids; 
    std::map<Price, std::list<Order>, PriceLess> asks; 
    std::map<Price, int, PriceGreater> bid_volume_at_price;
    std::map<Price, int, PriceLess> ask_volume_at_price;
    std::unordered_map<int, Order> order_location;

    void add_order(Order order_to_add);
    void update_order_volume(Order& order, int trade_volume);

    template <typename PriceComparator>
    void process_order(
	  Order& order_to_place,
	  const BookView<PriceComparator>& opposite_side_view,
	  std::vector<Orderbook::TradeInfo>& trades
    );
    
	bool is_valid_order(const Order& order) const;
    bool has_duplicate_id(const Order& order) const;
};

#endif
