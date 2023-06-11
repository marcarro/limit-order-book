#include <iostream>
#include <algorithm>
#include <stdlib.h>
#include <memory>

#include "Orderbook.h"

template <typename Compare>
void Orderbook::process_order(Order& order_to_place, std::map<double, std::list<Order>, Compare>& book, std::map<double, int>& volume_at_price, const std::function<bool(double, double)>& compare) {
  while (order_to_place.get_volume() > 0 && !book.empty() && compare(book.begin()->first, order_to_place.get_price())) {
    Order& other_order = book.begin()->second.front();

    int trade_volume = std::min(order_to_place.get_volume(), other_order.get_volume());

    volume_at_price[other_order.get_price()] = other_order.get_volume() - trade_volume;
    update_order_volume(other_order, trade_volume);
    update_order_volume(order_to_place, trade_volume);

    print_trade_information(other_order, order_to_place, trade_volume);

    if (other_order.get_volume() <= 0) cancel_order(other_order.get_order_id());
  }
}

void Orderbook::update_order_volume(Order& order, int trade_volume) {
  order.set_volume(order.get_volume() - trade_volume);
}

void Orderbook::print_trade_information(const Order& other_order, const Order& order_to_place, int trade_volume) {
  std::cout << other_order.get_client() << " " <<
  (order_to_place.get_side() == buy ? " --> " : " <-- ") <<
  order_to_place.get_client() << ", " << trade_volume << " shares @ "
  << other_order.get_price() << "\n";
}

void Orderbook::place_order(Order order_to_place) {
  order_location[order_to_place.get_order_id()] = order_to_place;
  if (order_to_place.get_side() == buy) {
    process_order(order_to_place, asks, ask_volume_at_price, std::less_equal<double>());
  } else {
    process_order(order_to_place, bids, bid_volume_at_price, std::greater_equal<double>());
  }
  if (order_to_place.get_volume() > 0) add_order(order_to_place);
}

void Orderbook::add_order(Order order_to_place) {
  if (order_to_place.get_side() == buy){
    bids[order_to_place.get_price()].push_back(order_to_place);
    bid_volume_at_price[order_to_place.get_price()] += order_to_place.get_volume();
  } else {
    asks[order_to_place.get_price()].push_back(order_to_place);
    ask_volume_at_price[order_to_place.get_price()] += order_to_place.get_volume();
  }
}

void Orderbook::cancel_order(int order_id) {
  if (order_location.find(order_id) == order_location.end()) return;
  buy_or_sell side = order_location[order_id].get_side();
  double key = order_location[order_id].get_price();
  int volume = order_location[order_id].get_volume();
  if (side == buy) {
    for (auto it = bids[key].begin(); it != bids[key].end();) {
      if (it->get_order_id() == order_id)
        it = bids[key].erase(it);
      else
        ++it;
    }
    if (bids[key].empty()) bids.erase(key);
    bid_volume_at_price[key] -= volume;
    if (bid_volume_at_price[key] <= 0) bid_volume_at_price.erase(key);
  } else {
    for (auto it = asks[key].begin(); it != asks[key].end();) {
      if (it->get_order_id() == order_id)
        it = asks[key].erase(it);
      else
        ++it;
    }
    if (asks[key].empty()) asks.erase(key);
    ask_volume_at_price[key] -= volume;
    if (ask_volume_at_price[key] <= 0) ask_volume_at_price.erase(key);
  }
  order_location.erase(order_id);
}

int Orderbook::get_volume_at_price(double price, buy_or_sell side) {
  if (side == buy) return bid_volume_at_price[price];
  else return ask_volume_at_price[price];
}

void Orderbook::view() {
  using namespace std;
  cout << "ASK\t Volume\t Price" << "\n";
  for (auto it = ask_volume_at_price.rbegin(); it != ask_volume_at_price.rend(); ++it) 
    cout << "\t " << it->second << "\t " << "\033[31m" << it->first << "\033[0m" << "\n";

  cout << "BID\t Volume\t Price" << "\n";
  for (auto it = bid_volume_at_price.rbegin(); it != bid_volume_at_price.rend(); ++it)
    cout << "\t " << it->second << "\t " << "\033[32m" << it->first << "\033[0m" << "\n";
  
  cout << "\n";
}