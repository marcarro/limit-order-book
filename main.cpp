#include <iostream>

#include "Orderbook.h"
#include "Benchmark.h"
#include "FixedPoint.h"

int main() {
  	Orderbook orderbook;
  	std::vector<Orderbook::TradeInfo> trades;
  	
  	{
    	Benchmark benchmark;
    	Order ord1("Alex G", Price("100.0000"), 1, 30, sell, time(0));
    	auto result = orderbook.place_order(ord1, trades);
		std::cout << "Order 1 result: " << static_cast<int>(result) << std::endl;
  	}
	
  	{
    	Benchmark benchmark;
    	Order ord2("John M", 101.0, 2, 30, sell, time(0));
    	auto result = orderbook.place_order(ord2, trades);
		std::cout << "Order 1 result: " << static_cast<int>(result) << std::endl;
  	}
	
  	// Print market data
    std::cout << "Best bid: " << orderbook.get_best_bid() << std::endl;
    std::cout << "Best ask: " << orderbook.get_best_ask() << std::endl;
    
    // Print book state
    orderbook.print_book();

	// Test fixed-point arithmetic
    Price price1("100.5000");
    Price price2("99.7500");
    Price sum = price1 + price2;
    Price diff = price1 - price2;
    
    std::cout << "Fixed-point tests:" << std::endl;
    std::cout << price1 << " + " << price2 << " = " << sum << std::endl;
    std::cout << price1 << " - " << price2 << " = " << diff << std::endl;
    std::cout << price1 << " / 2 = " << (price1 / 2) << std::endl;

  return 0;
}
