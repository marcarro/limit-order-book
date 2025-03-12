#ifndef ORDERBOOK_H
#define ORDERBOOK_H
#pragma once

#include <list>
#include <map>
#include <utility>
#include <unordered_map>

#include "Order.h"

enum class BookSide { Bid, Ask };

template <typename PriceComparator>
struct BookView {
	std::map<double, std::list<Order>, PriceComparator>& book;
	std::map<double, int>& volume_tracker;
	std::function<bool(double, double)> price_matches_condition;
};

class Orderbook {
private:
  // Acts as max heap through std::map::begin
  std::map<double, std::list<Order>, std::greater<double>> bids; 

  // Acts as min heap through std::map::begin
  std::map<double, std::list<Order>> asks; 

  // format: {price, volume}
  std::map<double, int> bid_volume_at_price;
  std::map<double, int> ask_volume_at_price;

  // format: {order_id, order} "side = map, price = key"
  std::unordered_map<int, Order> order_location;

  void add_order(Order order_to_add);
  void update_order_volume(Order& order, int trade_volume);
  void print_trade_information(const Order& other_order, const Order& order_to_place, int trade_volume);

  template <typename PriceComparator>
  void process_order(
	Order& order_to_place,
	const BookView<PriceComparator>& opposite_side_view
  );
public:
  void place_order(Order order_to_place);
  void cancel_order(int order_id);
  int get_volume_at_price(double price, buy_or_sell side);
  void view();
};


#endif
