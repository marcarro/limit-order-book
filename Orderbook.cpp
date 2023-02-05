#include <iostream>
#include <algorithm>

#include "Orderbook.h"

void Orderbook::place_order(Order order_to_place) {
  order_location[order_to_place.get_order_id()].first = order_to_place.get_side();
  order_location[order_to_place.get_order_id()].second = order_to_place.get_price();
  if (order_to_place.get_side() == buy) {
    while (order_to_place.get_volume() > 0 && !asks.empty() && asks.begin()->second.front().get_price() <= order_to_place.get_price()) {
      Order other_order = asks.begin()->second.front();

      std::cout << "other order addr: " << &other_order << std::endl;
      
      double trade_price = other_order.get_price();
      int trade_volume = std::min(order_to_place.get_volume(), other_order.get_volume());
      
      other_order.set_volume(other_order.get_volume() - trade_volume);
      order_to_place.set_volume(order_to_place.get_volume() - trade_volume);

      std::cout << other_order.get_client() << " --> " << 
      order_to_place.get_client() << ", " << trade_volume << " shares @ "
      << trade_price << std::endl;

      if (other_order.get_volume() == 0) cancel_order(other_order.get_order_id());
    }
    if (order_to_place.get_volume() > 0) add_order(order_to_place);
  } else {
    while (order_to_place.get_volume() > 0 && !bids.empty() && bids.begin()->second.front().get_price() <= order_to_place.get_price()) {
      Order other_order = bids.begin()->second.front();

      double trade_price = other_order.get_price();
      int trade_volume = std::min(order_to_place.get_volume(), other_order.get_volume());

      other_order.set_volume(other_order.get_volume() - trade_volume);
      order_to_place.set_volume(order_to_place.get_volume() - trade_volume);

      std::cout << other_order.get_client() << " --> " << 
      order_to_place.get_client() << ", " << trade_volume << " shares @ "
      << trade_price << std::endl;

      if (other_order.get_volume() == 0) cancel_order(other_order.get_order_id());
    }
    if (order_to_place.get_volume() > 0) add_order(order_to_place);
  }
}

void Orderbook::add_order(Order order_to_place) {
  if (order_to_place.get_side() == buy){
    bids[order_to_place.get_price()].emplace_back(order_to_place);
    bid_volume_at_price[order_to_place.get_price()]++;
  } else {
    asks[order_to_place.get_price()].emplace_back(order_to_place);
    ask_volume_at_price[order_to_place.get_price()]++;
  }
}

void Orderbook::cancel_order(int order_id) {
  if (order_location.find(order_id) == order_location.end()) return;
  buy_or_sell side = order_location[order_id].first;
  double key = order_location[order_id].second;
  if (side == buy) {
    for (auto it = bids[key].begin(); it != bids[key].end();) {
      if (it->get_order_id() == order_id)
        it = bids[key].erase(it);
      else
        ++it;
    }
    if (bids[key].empty()) bids.erase(key);
    bid_volume_at_price[key]--;
  } else {
    for (auto it = asks[key].begin(); it != asks[key].end();) {
      if (it->get_order_id() == order_id)
        it = asks[key].erase(it);
      else
        ++it;
    }
    if (asks[key].empty()) asks.erase(key);
    ask_volume_at_price[key]--;
  }
  order_location.erase(order_id);
}

int Orderbook::get_volume_at_price(double price, buy_or_sell side) {
  if (side == buy) return bid_volume_at_price[price];
  else return ask_volume_at_price[price];
}