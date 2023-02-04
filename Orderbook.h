#ifndef ORDERBOOK_H
#define ORDERBOOK_H
#pragma once

#include <queue>
#include <unordered_map>
#include <map>

#include "Order.h"

class Orderbook {
  std::map<double, std::queue<Order>> bids; // Acts as max heap through std::map::rbegin
  std::map<double, std::queue<Order>> asks; // Acts as min heap through std::map::begin
  std::unordered_map<double, int> volume_at_price; // format: {price, volume}
};

#endif