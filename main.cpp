#include <iostream>
#include "include/orderbook/Orderbook.h"
#include "include/orderbook/Order.h"
#include "include/utils/Benchmark.h"
#include "include/common/FixedPoint.h"

using namespace trading;

int main() {
	using namespace std::chrono;
  	Orderbook orderbook;
  	std::vector<TradeInfo> trades;
  	
  	{
    	Benchmark benchmark;
    	Order ord1("Alex G", Price("100.0000"), 1, 30, Side::SELL, system_clock::now());
    	auto result = orderbook.place_order(ord1, trades);
		std::cout << "Order 1 result: " << static_cast<int>(result) << std::endl;
  	}
	
  	{
    	Benchmark benchmark;
    	Order ord2("John M", 101.0, 2, 30, Side::SELL, system_clock::now());
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
