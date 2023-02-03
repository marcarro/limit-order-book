#ifndef ORDERBOOK_H
#define ORDERBOOK_H
#pragma once

#include <queue>
#include <unordered_map>

#include "Order.h"

class Orderbook {
  std::priority_queue<std::queue<Order>> bid; // Max heap
  std::priority_queue<std::queue<Order>, std::vector<std::queue<Order>>, std::greater<std::queue<Order>>> ask; // Min heap
  std::unordered_map<double, int> volume_at_price; // format: {price, volume}
};

#endif