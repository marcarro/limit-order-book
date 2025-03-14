#ifndef ORDER_H
#define ORDER_H
#pragma once

#include <ctime>
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
    std::time_t timestamp_;
public:

    Order* next = nullptr;
    Order* prev = nullptr;

    PriceLevel* level = nullptr;

    // Constructors
    Order();
    Order(std::string _client, Price _price, int _order_id, int _volume, Side _side, std::time_t _timestamp);

    // Compatibility constructor for easier transition (use with caution)
    Order(std::string _client, double _price, int _order_id, int _volume, Side _side, std::time_t _timestamp);
    
    // Getters
    std::string get_client() const;
    Price get_price() const;
    int get_order_id() const;
    int get_volume() const;
    Side get_side() const;
    std::time_t get_timestamp() const;
    
    // Setters
    void set_client(std::string new_client);
    void set_price(Price new_price);
    void set_order_id(int new_order_id);
    void set_volume(int new_volume);
    void set_side(Side new_side);
    void set_timestamp(std::time_t new_timestamp);
};

bool operator<(const Order& a, const Order& b);
bool operator>(const Order& a, const Order& b);

}


#endif
