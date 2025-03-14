#pragma once

#include "Orderbook.h"
#include "PriceLevel.h"
#include "../common/MemoryPool.h"
#include <unordered_map>

namespace trading {

class OrderbookImpl {
public:
	// Constructors / Destructors
	OrderbookImpl();
	~OrderbookImpl();

	// Core functionality
	OrderResult place_order(const Order& order, std::vector<TradeInfo>& trades);
    OrderResult cancel_order(int order_id);
    OrderResult modify_order(int order_id, const Price& new_price, int new_volume);

	// Market data access
	std::vector<BookLevel> get_bid_levels(int depth) const;
    std::vector<BookLevel> get_ask_levels(int depth) const;
    Price get_mid_price() const;
    Price get_best_bid() const;
    Price get_best_ask() const;
    int get_volume_at_price(const Price& price, Side side);

	// Metrics
	size_t order_count() const { return order_map_.size(); }
	size_t price_level_count() const;
    
	// Debug
    void print_book() const;
private:
	// Memory management
	memory::MemoryPool<Order> order_pool_;
	memory::MemoryPool<PriceLevel> level_pool_;

	// Order book structure
	PriceLevelList bid_levels_;
	PriceLevelList ask_levels_;

	// Direct lookup
	std::unordered_map<int, Order*> order_map_;

	// Order matching logic
    OrderResult match_against_asks(Order* order, std::vector<TradeInfo>& trades);
    OrderResult match_against_bids(Order* order, std::vector<TradeInfo>& trades);
    
    // Order management
    void add_order_to_book(Order* order);
    bool is_valid_order(const Order& order) const;
    bool has_duplicate_id(const Order& order) const;
};

}


