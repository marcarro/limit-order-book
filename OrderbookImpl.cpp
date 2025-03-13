#include <iostream>
#include "OrderbookImpl.h"

Orderbook::Result OrderbookImpl::place_order(const Order& order, std::vector<Orderbook::TradeInfo>& trades) {
	// Validate order first
    if (!is_valid_order(order)) {
        return Orderbook::Result::INVALID_ORDER;
    }
    
    if (has_duplicate_id(order)) {
        return Orderbook::Result::DUPLICATE_ORDER_ID;
    }

	// Store copy
	Order order_copy = order;
	int initial_volume = order_copy.get_volume();

	// Process order
  	order_location[order.get_order_id()] = order;

  	if (order.get_side() == buy) {
		BookView<std::less<double>> ask_view = {
			asks,
			ask_volume_at_price,
			std::less_equal<double>()
		};
    	process_order(order_copy, ask_view, trades);
  	} else {
		BookView<std::greater<double>> bid_view = {
			bids,
			bid_volume_at_price,
			std::greater_equal<double>()
		};
    	process_order(order_copy, bid_view, trades);
  	}

  	if (order_copy.get_volume() > 0) {
		add_order(order_copy);


		if (order_copy.get_volume() < initial_volume) {
			return Orderbook::Result::PARTIAL_FILL;
		} else {
			return Orderbook::Result::SUCCESS;
		}
	}

	return Orderbook::Result::COMPLETE_FILL;
}

Orderbook::Result OrderbookImpl::cancel_order(int order_id) {
  	auto order_it = order_location.find(order_id);
  	if (order_it == order_location.end()) {
	  	return Orderbook::Result::ORDER_NOT_FOUND;
  	}
	
  	Order& order = order_it->second;
  	buy_or_sell side = order.get_side();
  	double price = order.get_price();
  	int volume = order.get_volume();
	
  	if (side == buy) {
    	bid_volume_at_price[price] -= volume;
    	if (bid_volume_at_price[price] <= 0) 
			bid_volume_at_price.erase(price);
    	
    	auto& orders_at_price = bids[price];
    	orders_at_price.remove_if([order_id](const Order& o) {
			return o.get_order_id() == order_id; 
		});
	
    	if (orders_at_price.empty()) 
			bids.erase(price);
  	} else {
    	ask_volume_at_price[price] -= volume;
    	if (ask_volume_at_price[price] <= 0) 
			ask_volume_at_price.erase(price);
    	
    	auto& orders_at_price = asks[price];
    	orders_at_price.remove_if([order_id](const Order& o) {
			return o.get_order_id() == order_id; 
		});
	
    	if (orders_at_price.empty()) 
			asks.erase(price);
  	}
	
  	order_location.erase(order_id);
  	return Orderbook::Result::SUCCESS;
}

Orderbook::Result OrderbookImpl::modify_order(int order_id, double new_price, int new_volume) {
    // Find the order
    auto order_it = order_location.find(order_id);
    if (order_it == order_location.end()) {
        return Orderbook::Result::ORDER_NOT_FOUND;
    }
    
    // First cancel the existing order
    auto result = cancel_order(order_id);
    if (result != Orderbook::Result::SUCCESS) {
        return result;
    }
    
    // Create a new order with updated price/volume but same ID
    Order& old_order = order_it->second;
    Order new_order(
        old_order.get_client(),
        new_price,
        order_id,
        new_volume,
        old_order.get_side(),
        time(0) // Use current time
    );
    
    // Place the new order
    std::vector<Orderbook::TradeInfo> trades;
    return place_order(new_order, trades);
}

std::vector<Orderbook::BookLevel> OrderbookImpl::get_bid_levels(int depth) const {
    std::vector<Orderbook::BookLevel> levels;
    levels.reserve(std::min(static_cast<size_t>(depth), bids.size()));
    
    int count = 0;
    for (const auto& [price, orders] : bids) {
        if (count >= depth) break;
        
        Orderbook::BookLevel level;
        level.price = price;
        level.total_volume = bid_volume_at_price.at(price);
        level.order_count = orders.size();
        
        levels.push_back(level);
        count++;
    }
    
    return levels;
}

std::vector<Orderbook::BookLevel> OrderbookImpl::get_ask_levels(int depth) const {
    std::vector<Orderbook::BookLevel> levels;
    levels.reserve(std::min(static_cast<size_t>(depth), asks.size()));
    
    int count = 0;
    for (const auto& [price, orders] : asks) {
        if (count >= depth) break;
        
        Orderbook::BookLevel level;
        level.price = price;
        level.total_volume = ask_volume_at_price.at(price);
        level.order_count = orders.size();
        
        levels.push_back(level);
        count++;
    }
    
    return levels;
}

double OrderbookImpl::get_mid_price() const {
    double best_bid = get_best_bid();
    double best_ask = get_best_ask();
    
    if (best_bid <= 0 || best_ask <= 0) {
        return 0.0; // No valid mid price
    }
    
    return (best_bid + best_ask) / 2.0;
}

double OrderbookImpl::get_best_bid() const {
    if (bids.empty()) {
        return 0.0; // No bids
    }
    
    return bids.begin()->first;
}

double OrderbookImpl::get_best_ask() const {
    if (asks.empty()) {
        return 0.0; // No asks
    }
    
    return asks.begin()->first;
}

int OrderbookImpl::get_volume_at_price(double price, buy_or_sell side) const {
    if (side == buy) {
        auto it = bid_volume_at_price.find(price);
        return (it != bid_volume_at_price.end()) ? it->second : 0;
    } else {
        auto it = ask_volume_at_price.find(price);
        return (it != ask_volume_at_price.end()) ? it->second : 0;
    }
}

void OrderbookImpl::print_book() const {
    using namespace std;
    cout << "ASK\t Volume\t Price\t Orders" << "\n";
    cout << "----------------------------" << "\n";
    
    // Print asks in reverse order (highest to lowest)
    for (auto it = asks.rbegin(); it != asks.rend(); ++it) {
        cout << "\t " << ask_volume_at_price.at(it->first) 
             << "\t " << "\033[31m" << it->first << "\033[0m" 
             << "\t " << it->second.size() << "\n";
    }
    
    // Divider between asks and bids
    cout << "----------------------------" << "\n";
    
    // Print bids (highest to lowest)
    cout << "BID\t Volume\t Price\t Orders" << "\n";
    cout << "----------------------------" << "\n";
    for (auto it = bids.begin(); it != bids.end(); ++it) {
        cout << "\t " << bid_volume_at_price.at(it->first) 
             << "\t " << "\033[32m" << it->first << "\033[0m" 
             << "\t " << it->second.size() << "\n";
    }
    
    cout << "\n";
}


// Helper methods
void OrderbookImpl::update_order_volume(Order& order, int trade_volume) {
  	order.set_volume(order.get_volume() - trade_volume);
}

void OrderbookImpl::add_order(Order order_to_place) {
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

template <typename PriceComparator>
void OrderbookImpl::process_order(
	Order& order_to_place, 
	const BookView<PriceComparator>& opposite_side_view,
	std::vector<Orderbook::TradeInfo>& trades
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

		// Create trade info and add to trades vector
        Orderbook::TradeInfo trade_info;
        trade_info.order_id = order_to_place.get_order_id();
        trade_info.client_name = order_to_place.get_client();
        trade_info.price = matching_order.get_price();
        trade_info.volume = trade_volume;
        trade_info.is_buy = (order_to_place.get_side() == buy);
        trade_info.counterparty = matching_order.get_client();
        trades.push_back(trade_info);
	
    	if (matching_order.get_volume() <= 0) {
      		int order_id = matching_order.get_order_id();
	  		order_location.erase(order_id);
      		opposite_side_book.begin()->second.pop_front();
		
      		if (opposite_side_book.begin()->second.empty())
	    		opposite_side_book.erase(opposite_side_book.begin());
    	}
  	}
}

bool OrderbookImpl::is_valid_order(const Order& order) const {
    // Check for valid volume
	if (order.get_volume() <= 0) {
        return false;
    }
    
    // Check for valid price
    if (order.get_price() <= 0) {
        return false;
    }
    
    return true;
}

bool OrderbookImpl::has_duplicate_id(const Order& order) const {
    return order_location.find(order.get_order_id()) != order_location.end();
}

template void OrderbookImpl::process_order<std::less<double>>(
    Order&, const BookView<std::less<double>>&, std::vector<Orderbook::TradeInfo>&
);

template void OrderbookImpl::process_order<std::greater<double>>(
    Order&, const BookView<std::greater<double>>&, std::vector<Orderbook::TradeInfo>&
);
