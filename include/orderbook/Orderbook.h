#pragma once

#include <memory>
#include <vector>
#include "OrderbookTypes.h"
#include "Order.h"

namespace trading {

class OrderbookImpl;

class Orderbook {
public:
    // Constructor/destructor
    Orderbook();
    ~Orderbook();

	// Disable copying
    Orderbook(const Orderbook&) = delete;
    Orderbook& operator=(const Orderbook&) = delete;
    
    // Enable moving
    Orderbook(Orderbook&&) noexcept;
    Orderbook& operator=(Orderbook&&) noexcept;
    
    // Core functionality
    OrderResult place_order(const Order& order, std::vector<TradeInfo>& trades_executed);
    OrderResult cancel_order(int order_id);
    OrderResult modify_order(int order_id, const Price& new_price, int new_volume);

    OrderResult modify_order(int order_id, double new_price, int new_volume) {
		return modify_order(order_id, Price(new_price), new_volume);
	}
    
    // Market data access
    std::vector<BookLevel> get_bid_levels(int depth = 10) const;
    std::vector<BookLevel> get_ask_levels(int depth = 10) const;
    Price get_mid_price() const;
    Price get_best_bid() const;
    Price get_best_ask() const;
    int get_volume_at_price(const Price& price, Side side) const;

    int get_volume_at_price(double price, Side side) const {
		return get_volume_at_price(Price(price), side);
	}

	// Metrics
	size_t order_count() const;
	size_t price_level_count() const;
    
    // Debug functionality
    void print_book() const;
private:
	std::unique_ptr<OrderbookImpl> impl_;
};


}


