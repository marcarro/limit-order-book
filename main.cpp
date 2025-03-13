#include <iostream>

#include "Orderbook.h"
#include "Benchmark.h"

int main() {
  	Orderbook orderbook;
  	std::vector<Orderbook::TradeInfo> trades;
  	
  	{
    	Benchmark benchmark;
    	Order ord1("Alex G", 100, 1, 30, sell, time(0));
    	auto result = orderbook.place_order(ord1, trades);
		std::cout << "Order 1 result: " << static_cast<int>(result) << std::endl;
  	}
	
  	{
    	Benchmark benchmark;
    	Order ord2("John M", 101, 2, 30, sell, time(0));
    	auto result = orderbook.place_order(ord2, trades);
		std::cout << "Order 1 result: " << static_cast<int>(result) << std::endl;
  	}
	
  	// Print market data
    std::cout << "Best bid: " << orderbook.get_best_bid() << std::endl;
    std::cout << "Best ask: " << orderbook.get_best_ask() << std::endl;
    
    // Print book state
    orderbook.print_book();

  return 0;
}
