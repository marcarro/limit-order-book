#ifndef ORDERBOOK_H
#define ORDERBOOK_H
#pragma once

#include <list>
#include <map>
#include <utility>
#include <unordered_map>

#include "Order.h"

class Orderbook {
private:
  // Acts as max heap through std::map::begin
  std::map<double, std::list<Order>, std::greater<int>> bids; 

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
  template <typename Compare>
  void process_order(Order& order_to_place, std::map<double, std::list<Order>, Compare>& book, std::map<double, int>& volume_at_price, const std::function<bool(double, double)>& compare);
public:
  void place_order(Order order_to_place);
  void cancel_order(int order_id);
  int get_volume_at_price(double price, buy_or_sell side);
  void view();
};

#endif