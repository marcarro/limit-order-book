#include "../../include/orderbook/Orderbook.h"
#include <iostream>
#include <iomanip>

namespace trading {

Orderbook::Orderbook() :
    order_pool_(),
    level_pool_(),
    bid_levels_(true, level_pool_),  // true for bid side (descending prices)
    ask_levels_(false, level_pool_), // false for ask side (ascending prices)
    order_map_()
{}

Orderbook::~Orderbook() {
    // Clear all orders first (to avoid dangling pointers)
    order_map_.clear();
    
    // The memory pools will handle cleanup automatically
}


// Move constructor
Orderbook::Orderbook(Orderbook&& other) noexcept :
    order_pool_(std::move(other.order_pool_)),
    level_pool_(std::move(other.level_pool_)),
    bid_levels_(std::move(other.bid_levels_)),
    ask_levels_(std::move(other.ask_levels_)),
    order_map_(std::move(other.order_map_))
{
    bid_levels_.set_pool(level_pool_);
    ask_levels_.set_pool(level_pool_);
}

// Move assignment
Orderbook& Orderbook::operator=(Orderbook&& other) noexcept {
    if (this != &other) {
        order_pool_ = std::move(other.order_pool_);
        level_pool_ = std::move(other.level_pool_);
        bid_levels_ = std::move(other.bid_levels_);
        ask_levels_ = std::move(other.ask_levels_);
        order_map_ = std::move(other.order_map_);

        bid_levels_.set_pool(level_pool_);
        ask_levels_.set_pool(level_pool_);
    }
    return *this;
}
	
OrderResult Orderbook::place_order(const Order& order, std::vector<TradeInfo>& trades) {
    // Validate order first
    if (!is_valid_order(order)) {
        return OrderResult::INVALID_ORDER;
    }
    
    if (has_duplicate_id(order)) {
        return OrderResult::DUPLICATE_ORDER_ID;
    }

	// Allocate order
	Order* new_order = order_pool_.allocate();

	// Move order data
	new (new_order) Order(order);
	new_order->next = nullptr;
	new_order->prev = nullptr;
	new_order->level = nullptr;

	int initial_volume = new_order->get_volume();


	// Match order
    bool isBuyOrder = new_order->get_side() == Side::BUY;
    PriceLevelList& levelList = isBuyOrder ? ask_levels_ : bid_levels_;
    match_orders(new_order, trades, levelList, isBuyOrder);

	// If any volume remains, add to book
	if (new_order->get_volume() > 0) {
		add_order_to_book(new_order);

		order_map_[new_order->get_order_id()] = new_order;

		if (new_order->get_volume() < initial_volume) {
			return OrderResult::PARTIAL_FILL;
		} else {
			return OrderResult::SUCCESS;
		}
	} else {
		// Fully filled so deallocate
		order_pool_.deallocate(new_order);
		return OrderResult::COMPLETE_FILL;
	}

}

OrderResult Orderbook::cancel_order(int order_id) {
  	auto it = order_map_.find(order_id);
  	if (it == order_map_.end()) {
	  	return OrderResult::ORDER_NOT_FOUND;
  	}

	Order* order = it->second;
	PriceLevel* level = order->level;

	// Remove from price level
	if (level) {
		level->remove_order(order);

		// If price level is empty remove the level
		if (level->get_order_count() == 0) {
			if (order->get_side() == Side::BUY) {
				bid_levels_.remove_level(level);
			} else {
				ask_levels_.remove_level(level);
			}

			level_pool_.deallocate(level);
		}
	}

	// Remove from order map
	order_map_.erase(order_id);

	// Return order to pool
	order_pool_.deallocate(order);

	return OrderResult::SUCCESS;
}

OrderResult Orderbook::modify_order(int order_id, const Price& new_price, int new_volume) {
    // Find the order
    auto it = order_map_.find(order_id);
    if (it == order_map_.end()) {
        return OrderResult::ORDER_NOT_FOUND;
    }

	Order* order = it->second;

	// Handle volume reduction and no price change
	if (new_price == order->get_price() && new_volume < order->get_volume()) {
        int old_volume = order->get_volume();
        order->set_volume(new_volume);
        
        // Update volume at price level
        if (order->level) {
            order->level->update_volume(order, old_volume);
        }
        
        return OrderResult::SUCCESS;
    }

	// For all other cases, treat as cancel + new order
	Side side = order->get_side();
	std::string client = order->get_client();
	std::chrono::system_clock::time_point timestamp = std::chrono::system_clock::now();

	// Cancel existing order
    auto result = cancel_order(order_id);
    if (result != OrderResult::SUCCESS) {
        return result;
    }
    
    // Create a new order with same ID
    Order new_order(
        client,
        new_price,
        order_id,
        new_volume,
        side,
		timestamp
    );
    
    // Place the new order
    std::vector<TradeInfo> trades;
    return place_order(new_order, trades);
}

OrderResult Orderbook::match_orders(Order* order, std::vector<TradeInfo>& trades, PriceLevelList& levels, bool is_buy_order) {
    bool any_match = false;
    int remaining_order_volume = order->get_volume();

    while (remaining_order_volume > 0 && !levels.empty()) {
        PriceLevel* best_level = levels.get_best_level();

        bool price_mismatch = is_buy_order
            ? best_level->get_price() > order->get_price()
            : best_level->get_price() < order->get_price();

        if (price_mismatch) {
            break;
        }

        Order* matching_order = best_level->head;

        while (matching_order && remaining_order_volume > 0) {
            int matching_volume = matching_order->get_volume();
            int trade_volume = std::min(remaining_order_volume, matching_volume);
            any_match = true;

            int new_matching_volume = matching_volume - trade_volume;
            int new_order_volume = remaining_order_volume - trade_volume;

            TradeInfo trade;
            trade.order_id = order->get_order_id();
            trade.client_name = order->get_client();
            trade.price = matching_order->get_price();
            trade.volume = trade_volume;
            trade.is_buy = (order->get_side() == Side::BUY);
            trade.counterparty = matching_order->get_client();
            trades.push_back(trade);

            Order* next_matching = matching_order->next;

            if (new_matching_volume > 0) {
                matching_order->set_volume(new_matching_volume);
                best_level->update_volume(matching_order, matching_volume);
            } else {
                int matching_id = matching_order->get_order_id();
                best_level->remove_order(matching_order);
                order_map_.erase(matching_id);
                order_pool_.deallocate(matching_order);
            }

            order->set_volume(new_order_volume);
            remaining_order_volume = new_order_volume;

            matching_order = next_matching;
        }

        if (best_level->get_order_count() == 0) {
            PriceLevel* level_to_remove = best_level;
            levels.remove_level(level_to_remove);
            level_pool_.deallocate(level_to_remove);
        }
    }

    if (order->get_volume() == 0) {
        return OrderResult::COMPLETE_FILL;
    } else if (any_match) {
        return OrderResult::PARTIAL_FILL;
    } else {
        return OrderResult::SUCCESS;
    }
}

OrderResult Orderbook::match_against_asks(Order* order, std::vector<TradeInfo>& trades) {
    return match_orders(order, trades, ask_levels_, true);
}

OrderResult Orderbook::match_against_bids(Order* order, std::vector<TradeInfo>& trades) {
    return match_orders(order, trades, bid_levels_, false);
}

void Orderbook::add_order_to_book(Order* order) {
    Price price = order->get_price();
    PriceLevel* level = (order->get_side() == Side::BUY)
        ? bid_levels_.find_or_create_level(price)
        : ask_levels_.find_or_create_level(price);
    
    level->add_order(order);
}

std::vector<BookLevel> Orderbook::get_bid_levels(int depth) const {
    std::vector<BookLevel> result;
    result.reserve(depth);
    
	PriceLevel* level = bid_levels_.begin();
    int count = 0;

    while (level && count < depth) {
		BookLevel book_level;
		book_level.price = level->get_price();
		book_level.total_volume = level->get_total_volume();
		book_level.order_count = level->get_order_count();

		result.push_back(book_level);

		level = bid_levels_.next(level);
		count++;
    }
    
    return result;
}

std::vector<BookLevel> Orderbook::get_ask_levels(int depth) const {
    std::vector<BookLevel> result;
    result.reserve(depth);
    
	PriceLevel* level = ask_levels_.begin();
    int count = 0;

    while (level && count < depth) {
		BookLevel book_level;
		book_level.price = level->get_price();
		book_level.total_volume = level->get_total_volume();
		book_level.order_count = level->get_order_count();

		result.push_back(book_level);

		level = ask_levels_.next(level);
		count++;
    }
    
    return result;
}

Price Orderbook::get_mid_price() const {
    Price best_bid = get_best_bid();
    Price best_ask = get_best_ask();
    
    if (best_bid.raw_value() <= 0 || best_ask.raw_value() <= 0) {
        return Price(); // No valid mid price
    }
    
    return (best_bid + best_ask) / 2;
}

Price Orderbook::get_best_bid() const {
	if (bid_levels_.empty()) {
		return Price(0);
	}

	return bid_levels_.get_best_level()->get_price();
}

Price Orderbook::get_best_ask() const {
    if (ask_levels_.empty()) {
        return Price(0); // No asks
    }
    
    return ask_levels_.get_best_level()->get_price();
}

int Orderbook::get_volume_at_price(const Price& price, Side side) const {
    if (side == Side::BUY) {
		PriceLevel* level = bid_levels_.find_level(price);
		return level ? level->get_total_volume() : 0;
    } else {
		PriceLevel* level = ask_levels_.find_level(price);
		return level ? level->get_total_volume() : 0;
    }
}

size_t Orderbook::price_level_count() const {
    size_t count = 0;
    
    // Count bid levels
    PriceLevel* bid_level = bid_levels_.begin();
    while (bid_level) {
        count++;
        bid_level = bid_levels_.next(bid_level);
    }
    
    // Count ask levels
    PriceLevel* ask_level = ask_levels_.begin();
    while (ask_level) {
        count++;
        ask_level = ask_levels_.next(ask_level);
    }
    
    return count;
}

void Orderbook::reserve_orders(size_t live_orders) {
    order_pool_.reserve(live_orders);
    order_map_.reserve(live_orders);
}

void Orderbook::reserve_price_levels(size_t total_levels) {
    level_pool_.reserve(total_levels);
}

void Orderbook::print_book() const {
    std::cout << "--------- ORDER BOOK ---------" << std::endl;
    std::cout << "ASKS:" << std::endl;
    std::cout << std::setw(10) << "Price" << " | " 
              << std::setw(10) << "Volume" << " | " 
              << std::setw(5) << "Orders" << std::endl;
    std::cout << "-----------------------------" << std::endl;
    
    // Print asks in reverse order (highest to lowest)
    std::vector<BookLevel> asks = get_ask_levels(10);
    for (auto it = asks.rbegin(); it != asks.rend(); ++it) {
        std::cout << std::setw(10) << it->price.to_string() << " | " 
                  << std::setw(10) << it->total_volume << " | " 
                  << std::setw(5) << it->order_count << std::endl;
    }
    
    std::cout << "-----------------------------" << std::endl;
    
    // Print bids (highest to lowest)
    std::cout << "BIDS:" << std::endl;
    std::cout << std::setw(10) << "Price" << " | " 
              << std::setw(10) << "Volume" << " | " 
              << std::setw(5) << "Orders" << std::endl;
    std::cout << "-----------------------------" << std::endl;
    
    std::vector<BookLevel> bids = get_bid_levels(10);
    for (const auto& level : bids) {
        std::cout << std::setw(10) << level.price.to_string() << " | " 
                  << std::setw(10) << level.total_volume << " | " 
                  << std::setw(5) << level.order_count << std::endl;
    }
    
    std::cout << "-----------------------------" << std::endl;
}


// Helper methods
bool Orderbook::is_valid_order(const Order& order) const {
    // Check for valid volume
	if (order.get_volume() <= 0) {
        return false;
    }
    
    // Check for valid price
    if (order.get_price().raw_value() <= 0) {
        return false;
    }
    
    return true;
}

bool Orderbook::has_duplicate_id(const Order& order) const {
    return order_map_.find(order.get_order_id()) != order_map_.end();
}

}
