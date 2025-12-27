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
    Order(std::string _client, Price _price, int _order_id, int _volume, Side _side, std::chrono::system_clock::time_point _timestamp);

    // Auto-generated order_id, manual timestamp
    Order(std::string _client, Price _price, int _volume, Side _side, std::chrono::system_clock::time_point _timestamp);

    // Auto-generated order_id and timestamp
    Order(std::string _client, Price _price, int _volume, Side _side);

    // Compatibility constructors with double price
    Order(std::string _client, double _price, int _order_id, int _volume, Side _side, std::chrono::system_clock::time_point _timestamp);
    Order(std::string _client, double _price, int _volume, Side _side, std::chrono::system_clock::time_point _timestamp);
    Order(std::string _client, double _price, int _volume, Side _side);

    // Reset auto-generated order ID counter (useful for tests)
    static void reset_order_id_counter(int start_id = 1);
    
    // Getters
    std::string get_client() const;
    Price get_price() const;
    int get_order_id() const;
    int get_volume() const;
    Side get_side() const;
    std::chrono::system_clock::time_point get_timestamp() const;
    
    // Setters
    void set_client(std::string new_client);
    void set_price(Price new_price);
    void set_order_id(int new_order_id);
    void set_volume(int new_volume);
    void set_side(Side new_side);
    void set_timestamp(std::chrono::system_clock::time_point new_timestamp);
};

bool operator<(const Order& a, const Order& b);
bool operator>(const Order& a, const Order& b);

}


#endif
