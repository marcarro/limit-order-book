#include "Order.h"

// Getters
std::string Order::get_client() { return client; }
double Order::get_price() { return price; }
int Order::get_order_id() { return order_id; }
int Order::get_volume() { return volume; }
buy_or_sell Order::get_side() { return side; }
std::time_t Order::get_timestamp() { return timestamp; }

// Setters

void Order::set_client(std::string new_client) { client = new_client; }
void Order::set_price(double new_price) { price = new_price; }
void Order::set_order_id(int new_order_id) { order_id = new_order_id; }
void Order::set_volume(int new_volume) { volume = new_volume; }
void Order::set_side(buy_or_sell new_side) { side = new_side; }
void Order::set_timestamp(std::time_t new_timestamp) { timestamp = new_timestamp; }