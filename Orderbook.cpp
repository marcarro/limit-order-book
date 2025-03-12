#include <iostream>
#include <stdlib.h>

#include "Orderbook.h"

template <typename PriceComparator>
void Orderbook::process_order(
	Order& order_to_place, 
	const BookView<PriceComparator>& opposite_side_view
) {
    auto& opposite_side_book = opposite_side_view.book;
	auto& opposite_volume_tracker = opposite_side_view.volume_tracker;
	auto& price_matches_condition = opposite_side_view.price_matches_condition;

  while (order_to_place.get_volume() > 0 && !opposite_side_book.empty()) {
    if (!price_matches_condition(opposite_side_book.begin()->first, order_to_place.get_price())) 
		break;

    Order& matching_order = opposite_side_book.begin()->second.front();
    int trade_volume = std::min(order_to_place.get_volume(), matching_order.get_volume());

    opposite_volume_tracker[matching_order.get_price()] -= trade_volume;

    update_order_volume(matching_order, trade_volume);
    update_order_volume(order_to_place, trade_volume);

    print_trade_information(matching_order, order_to_place, trade_volume);

    if (matching_order.get_volume() <= 0) {
      int order_id = matching_order.get_order_id();
	  order_location.erase(order_id);
      opposite_side_book.begin()->second.pop_front();

      if (opposite_side_book.begin()->second.empty())
	    opposite_side_book.erase(opposite_side_book.begin());
    }
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
	BookView<std::less<double>> ask_view = {
		asks,
		ask_volume_at_price,
		std::less_equal<double>()
	};
    process_order(order_to_place, ask_view);
  } else {
	BookView<std::greater<double>> bid_view = {
		bids,
		bid_volume_at_price,
		std::greater_equal<double>()
	};
    process_order(order_to_place, bid_view);
  }

  if (order_to_place.get_volume() > 0)
	  add_order(order_to_place);
}

void Orderbook::add_order(Order order_to_place) {
  double price = order_to_place.get_price();
  int volume = order_to_place.get_volume();

  if (order_to_place.get_side() == buy){
	bid_volume_at_price[price] += volume;
    bids[price].push_back(order_to_place);
  } else {
    ask_volume_at_price[price] += volume;
    asks[price].push_back(order_to_place);
  }
}

void Orderbook::cancel_order(int order_id) {
  auto order_it = order_location.find(order_id);
  if (order_it == order_location.end()) return;

  Order& order = order_it->second;
  buy_or_sell side = order.get_side();
  double price = order.get_price();
  int volume = order.get_volume();

  if (side == buy) {
    bid_volume_at_price[price] -= volume;
    if (bid_volume_at_price[price] <= 0) bid_volume_at_price.erase(price);
    
    auto& orders_at_price = bids[price];
    orders_at_price.remove_if([order_id](const Order& o) { return o.get_order_id() == order_id; });
    if (orders_at_price.empty()) bids.erase(price);
  } else {
    ask_volume_at_price[price] -= volume;
    if (ask_volume_at_price[price] <= 0) ask_volume_at_price.erase(price);
    
    auto& orders_at_price = asks[price];
    orders_at_price.remove_if([order_id](const Order& o) { return o.get_order_id() == order_id; });
    if (orders_at_price.empty()) asks.erase(price);
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

template void Orderbook::process_order<std::less<double>>(
    Order&, const BookView<std::less<double>>&
);

template void Orderbook::process_order<std::greater<double>>(
    Order&, const BookView<std::greater<double>>&
);
