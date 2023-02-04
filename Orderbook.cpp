#include "Orderbook.h"

void Orderbook::place_order(Order order_to_place) {
  if (order_to_place.get_side() == buy){
    bids[order_to_place.get_price()].emplace_back(order_to_place);
    bid_volume_at_price[order_to_place.get_price()]++;
  } else {
    asks[order_to_place.get_price()].emplace_back(order_to_place);
    ask_volume_at_price[order_to_place.get_price()]++;
  }
  order_location[order_to_place.get_order_id()].first = order_to_place.get_side();
  order_location[order_to_place.get_order_id()].second = order_to_place.get_price();
}

void Orderbook::cancel_order(int order_id) {
  buy_or_sell side = order_location[order_id].first;
  double key = order_location[order_id].second;
  if (side == buy) {
    for (auto it = bids[key].begin(); it != bids[key].end();) {
      if (it->get_order_id() == order_id)
        it = bids[key].erase(it);
      else
        ++it;
    }
    bid_volume_at_price[key]--;
  } else {
    for (auto it = asks[key].begin(); it != asks[key].end();) {
      if (it->get_order_id() == order_id)
        it = asks[key].erase(it);
      else
        ++it;
    }
    ask_volume_at_price[key]--;
  }
  order_location.erase(order_id);
}

int Orderbook::get_volume_at_price(double price, buy_or_sell side) {
  if (side == buy) return bid_volume_at_price[price];
  else return ask_volume_at_price[price];
}