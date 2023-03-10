#ifndef ORDERBOOK_H
#define ORDERBOOK_H
#pragma once

#include <list>
#include <map>
#include <utility>
#include <unordered_map>

#include "Order.h"

class Orderbook {
public:
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
public:
  void place_order(Order order_to_place);
  template <typename T> void place_order2(Order order_to_place);
  void cancel_order(int order_id);
  int get_volume_at_price(double price, buy_or_sell side);
  void view();
};

#endif