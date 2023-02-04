#ifndef ORDERBOOK_H
#define ORDERBOOK_H
#pragma once

#include <queue>
#include <unordered_map>
#include <map>

#include "Order.h"

class Orderbook {
private:
  std::map<double, std::queue<Order>> bids; // Acts as max heap through std::map::rbegin
  std::map<double, std::queue<Order>> asks; // Acts as min heap through std::map::begin
  std::unordered_map<double, int> volume_at_price; // format: {price, volume}
public:
  void place_order(Order order_to_place);
  void cancel_order(int order_id);
  int get_volume_at_price(double price, buy_or_sell side);
};

#endif