#ifndef ORDERBOOK_IMPL_H
#define ORDERBOOK_IMPL_H
#pragma once

#include <list>
#include <map>
#include <utility>
#include <functional>
#include "Order.h"
#include "Orderbook.h"

template <typename PriceComparator>
struct BookView {
	std::map<double, std::list<Order>, PriceComparator>& book;
	std::map<double, int>& volume_tracker;
	std::function<bool(double, double)> price_matches_condition;
};

class OrderbookImpl {
public:
	Orderbook::Result place_order(const Order& order, std::vector<Orderbook::TradeInfo>& trades);
    Orderbook::Result cancel_order(int order_id);
    Orderbook::Result modify_order(int order_id, double new_price, int new_volume);

	std::vector<Orderbook::BookLevel> get_bid_levels(int depth) const;
    std::vector<Orderbook::BookLevel> get_ask_levels(int depth) const;
    double get_mid_price() const;
    double get_best_bid() const;
    double get_best_ask() const;
    int get_volume_at_price(double price, buy_or_sell side) const;
    
    void print_book() const;
private:
    std::map<double, std::list<Order>, std::greater<double>> bids; 
    std::map<double, std::list<Order>> asks; 
    std::map<double, int> bid_volume_at_price;
    std::map<double, int> ask_volume_at_price;
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
