#ifndef ORDERBOOK_H
#define ORDERBOOK_H
#pragma once

#include <memory>
#include <vector>
#include "Order.h"

class OrderbookImpl;

class Orderbook {
public:
	// Result types for operations
	enum class Result {
		SUCCESS,
		PARTIAL_FILL,
		COMPLETE_FILL,
		REJECTED,
		INVALID_ORDER,
		ORDER_NOT_FOUND,
		DUPLICATE_ORDER_ID
	};

	// Trade information returned after placement
	struct TradeInfo {
		int order_id;
		std::string client_name;
		double price;
		int volume;
		bool is_buy;
		std::string counterparty;
	};

	// Book level information for market data
	struct BookLevel {
		double price;
		int total_volume;
		int order_count;
	};
  	
    // Constructor/destructor
    Orderbook();
    ~Orderbook();
    
    // Core functionality
    Result place_order(const Order& order, std::vector<TradeInfo>& trades_executed);
    Result cancel_order(int order_id);
    Result modify_order(int order_id, double new_price, int new_volume);
    
    // Market data access
    std::vector<BookLevel> get_bid_levels(int depth = 10) const;
    std::vector<BookLevel> get_ask_levels(int depth = 10) const;
    double get_mid_price() const;
    double get_best_bid() const;
    double get_best_ask() const;
    int get_volume_at_price(double price, buy_or_sell side) const;
    
    // Debug functionality
    void print_book() const;
private:
	std::unique_ptr<OrderbookImpl> impl_;
};

#endif
