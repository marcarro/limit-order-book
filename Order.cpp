#include "Order.h"

// Constructors
Order::Order(): 
	client{},
	price{},
	order_id{0},
	volume{0},
	side{buy},
	timestamp{}
{}

Order::Order(std::string _client, Price _price, int _order_id, int _volume, buy_or_sell _side, std::time_t _timestamp):
	client{_client},
	price{_price},
	order_id{_order_id},
	volume{_volume},
	side{_side},
	timestamp{_timestamp}
{}

Order::Order(std::string _client, double _price, int _order_id, int _volume, buy_or_sell _side, std::time_t _timestamp):
	client{_client},
	price{Price(_price)},  // Convert double to Price
	order_id{_order_id},
	volume{_volume},
	side{_side},
	timestamp{_timestamp}
{}

// Operator overload for min and max heap
bool operator<(const Order& a, const Order& b) {
  return a.price < b.price;
}

bool operator>(const Order& a, const Order& b) {
  return b < a;
}

// Getters
std::string Order::get_client() const { return client; }
Price Order::get_price() const { return price; }
int Order::get_order_id() const { return order_id; }
int Order::get_volume() const { return volume; }
buy_or_sell Order::get_side() const { return side; }
std::time_t Order::get_timestamp() const { return timestamp; }

// Setters
void Order::set_client(std::string new_client) { client = new_client; }
void Order::set_price(Price new_price) { price = new_price; }
void Order::set_order_id(int new_order_id) { order_id = new_order_id; }
void Order::set_volume(int new_volume) { volume = new_volume; }
void Order::set_side(buy_or_sell new_side) { side = new_side; }
void Order::set_timestamp(std::time_t new_timestamp) { timestamp = new_timestamp; }
