#include "../../include/orderbook/Order.h"

namespace trading {

// Constructors
Order::Order(): 
	client_(""),
	price_(0),
	order_id_(0),
	volume_(0),
	side_(Side::BUY),
	timestamp_(std::chrono::system_clock::now()),
	next(nullptr),
	prev(nullptr),
	level(nullptr)
{}

Order::Order(std::string _client, Price _price, int _order_id, int _volume, Side _side, std::chrono::system_clock::time_point _timestamp):
	client_{_client},
	price_{_price},
	order_id_{_order_id},
	volume_{_volume},
	side_{_side},
	timestamp_{_timestamp}
{}

Order::Order(std::string _client, double _price, int _order_id, int _volume, Side _side, std::chrono::system_clock::time_point _timestamp):
    client_(_client),
    price_(Price(_price)),
    order_id_(_order_id),
    volume_(_volume),
    side_(_side),
    timestamp_(_timestamp),
    next(nullptr),
    prev(nullptr),
    level(nullptr)
{}

// Getters
std::string Order::get_client() const { return client_; }
Price Order::get_price() const { return price_; }
int Order::get_order_id() const { return order_id_; }
int Order::get_volume() const { return volume_; }
Side Order::get_side() const { return side_; }
std::chrono::system_clock::time_point Order::get_timestamp() const { return timestamp_; }

// Setters
void Order::set_client(std::string new_client) { client_ = new_client; }
void Order::set_price(Price new_price) { price_ = new_price; }
void Order::set_order_id(int new_order_id) { order_id_ = new_order_id; }
void Order::set_volume(int new_volume) { volume_ = new_volume; }
void Order::set_side(Side new_side) { side_ = new_side; }
void Order::set_timestamp(std::chrono::system_clock::time_point new_timestamp) { timestamp_ = new_timestamp; }

}

