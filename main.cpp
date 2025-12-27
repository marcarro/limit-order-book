#include <iostream>
#include <random>
#include "include/orderbook/Orderbook.h"
#include "include/orderbook/Order.h"
#include "include/utils/Benchmark.h"
#include "include/common/FixedPoint.h"

using namespace trading;

int main() {
	Orderbook orderbook;
	std::vector<TradeInfo> trades;

	std::random_device rd;
	std::mt19937 gen(42);
	std::uniform_real_distribution<> price_dist(99.0, 101.0);
	std::uniform_int_distribution<> volume_dist(10, 500);
	std::uniform_int_distribution<> side_dist(0, 1);

	std::vector<std::string> clients = {
		"Goldman Sachs", "JPMorgan", "Morgan Stanley",
		"Bridgewater", "DE Shaw", "Millennium", "Point72", 
		"Balyasny", "Susquehanna", "IMC", "Flow Traders"
	};

	std::cout << "=== Order Book ===\n\n";

	{
		Benchmark benchmark;

		// Place 500000 orders around mid-price of 100
		for (int i = 0; i < 50000; i++) {
			std::string client = clients[i % clients.size()];
			double price = price_dist(gen);
			int volume = volume_dist(gen);
			Side side = side_dist(gen) == 0 ? Side::BUY : Side::SELL;

			Order order(client, price, volume, side);
			orderbook.place_order(order, trades);
		}
	}

	std::cout << "\n=== Order Book Statistics ===\n";
	std::cout << "Total orders in book: " << orderbook.order_count() << "\n";
	std::cout << "Total price levels: " << orderbook.price_level_count() << "\n";
	std::cout << "Total trades executed: " << trades.size() << "\n";
	std::cout << "Best bid: " << orderbook.get_best_bid().to_string() << "\n";
	std::cout << "Best ask: " << orderbook.get_best_ask().to_string() << "\n";
	std::cout << "Mid price: " << orderbook.get_mid_price().to_string() << "\n";

	if (!trades.empty()) {
		int total_volume = 0;
		for (const auto& trade : trades) {
			total_volume += trade.volume;
		}
		std::cout << "Total volume traded: " << total_volume << "\n";
	}

	std::cout << "\n=== Top 10 Levels ===\n";
	orderbook.print_book();

	// Demonstrate order operations
	std::cout << "\n=== Testing Order Operations ===\n";

	// Place a large buy order that will cross the spread
	std::cout << "\nPlacing aggressive buy order (500 @ 100.50)...\n";
	trades.clear();
	Order aggressive_buy("Aggressive Trader", 100.50, 500, Side::BUY);
	auto result = orderbook.place_order(aggressive_buy, trades);
	std::cout << "Result: " << static_cast<int>(result) << "\n";
	std::cout << "Trades executed: " << trades.size() << "\n";

	if (!trades.empty()) {
		std::cout << "First trade: " << trades[0].volume << " @ "
		          << trades[0].price.to_string() << " with "
		          << trades[0].counterparty << "\n";
	}

	// Place a passive order that rests in the book
	std::cout << "\nPlacing passive sell order (100 @ 100.80)...\n";
	trades.clear();
	Order passive_sell("Market Maker", 100.80, 100, Side::SELL);
	result = orderbook.place_order(passive_sell, trades);
	std::cout << "Result: " << static_cast<int>(result) << "\n";
	std::cout << "Order resting in book\n";

	std::cout << "\n=== Final Book State ===\n";
	std::cout << "Total orders: " << orderbook.order_count() << "\n";
	std::cout << "Best bid: " << orderbook.get_best_bid().to_string() << "\n";
	std::cout << "Best ask: " << orderbook.get_best_ask().to_string() << "\n";
	std::cout << "Spread: " << (orderbook.get_best_ask() - orderbook.get_best_bid()).to_string() << "\n";

	return 0;
}
