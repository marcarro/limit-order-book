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
  // Acts as max heap through std::map::rbegin
  std::map<double, std::list<Order>> bids; 

  // Acts as min heap through std::map::begin
  std::map<double, std::list<Order>> asks; 

  // format: {price, volume}
  std::unordered_map<double, int> bid_volume_at_price;
  std::unordered_map<double, int> ask_volume_at_price;

  // format: {order_id, {side, price}} "side = map, price = key"
  std::unordered_map<int, std::pair<buy_or_sell, double>> order_location;
public:
  void place_order(Order order_to_place);
  void cancel_order(int order_id);
  int get_volume_at_price(double price, buy_or_sell side);
};

#endif