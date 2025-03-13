#ifndef ORDERBOOK_H
#define ORDERBOOK_H
#pragma once

#include <memory>
#include <vector>
#include "Order.h"
#include "FixedPoint.h"

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
		Price price;
		int volume;
		bool is_buy;
		std::string counterparty;
	};

	// Book level information for market data
	struct BookLevel {
		Price price;
		int total_volume;
		int order_count;
	};
  	
    // Constructor/destructor
    Orderbook();
    ~Orderbook();
    
    // Core functionality
    Result place_order(const Order& order, std::vector<TradeInfo>& trades_executed);
    Result cancel_order(int order_id);
    Result modify_order(int order_id, const Price& new_price, int new_volume);

    Result modify_order(int order_id, double new_price, int new_volume) {
		return modify_order(order_id, Price(new_price), new_volume);
	}
    
    // Market data access
    std::vector<BookLevel> get_bid_levels(int depth = 10) const;
    std::vector<BookLevel> get_ask_levels(int depth = 10) const;
    Price get_mid_price() const;
    Price get_best_bid() const;
    Price get_best_ask() const;
    int get_volume_at_price(const Price& price, buy_or_sell side) const;

    int get_volume_at_price(double price, buy_or_sell side) const {
		return get_volume_at_price(Price(price), side);
	}
    
    // Debug functionality
    void print_book() const;
private:
	std::unique_ptr<OrderbookImpl> impl_;
};

#endif
