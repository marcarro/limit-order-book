#ifndef ORDER_H
#define ORDER_H
#pragma once

#include <chrono>
#include <string>
#include "OrderbookTypes.h"
#include "../common/FixedPoint.h"

namespace trading {

class PriceLevel;

class Order {
private:
  	std::string client_;
    Price price_;
    int order_id_;
    int volume_;
    Side side_;
    std::chrono::system_clock::time_point timestamp_;

    static int next_order_id_;
public:

    Order* next = nullptr;
    Order* prev = nullptr;

    PriceLevel* level = nullptr;

    // Constructors
    Order();

    // Full control constructor
    Order(const std::string& _client, Price _price, int _order_id, int _volume, Side _side, std::chrono::system_clock::time_point _timestamp);

    // Auto-generated order_id, manual timestamp
    Order(const std::string& _client, Price _price, int _volume, Side _side, std::chrono::system_clock::time_point _timestamp);

    // Auto-generated order_id and timestamp
    Order(const std::string& _client, Price _price, int _volume, Side _side);

    // Compatibility constructors with double price
    Order(const std::string& _client, double _price, int _order_id, int _volume, Side _side, std::chrono::system_clock::time_point _timestamp);
    Order(const std::string& _client, double _price, int _volume, Side _side, std::chrono::system_clock::time_point _timestamp);
    Order(const std::string& _client, double _price, int _volume, Side _side);

    // Reset auto-generated order ID counter (useful for tests)
    static void reset_order_id_counter(int start_id = 1);
    
    // Getters
    const std::string& get_client() const { return client_; }
    Price get_price() const { return price_; }
    int get_order_id() const { return order_id_; }
    int get_volume() const { return volume_; }
    Side get_side() const { return side_; }
    std::chrono::system_clock::time_point get_timestamp() const { return timestamp_; }
    
    // Setters
    void set_client(std::string new_client);
    void set_price(Price new_price);
    void set_order_id(int new_order_id);
    void set_volume(int new_volume);
    void set_side(Side new_side);
    void set_timestamp(std::chrono::system_clock::time_point new_timestamp);
};

}


#endif
