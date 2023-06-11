#ifndef ORDER_H
#define ORDER_H
#pragma once

#include <ctime>
#include <string>

enum buy_or_sell {
  buy,
  sell
};

class Order {
private:
  std::string client;
  double price;
  int order_id;
  int volume;
  buy_or_sell side;
  std::time_t timestamp;
public:

  // Constructors
  Order();
  Order(std::string _client, double _price, int _order_id, int _volume, buy_or_sell _side, std::time_t _timestamp);

  // Operator overload for min and max heap
  friend bool operator<(const Order& a, const Order& b);
  friend bool operator>(const Order& a, const Order& b);
  
  // Getters
  std::string get_client() const;
  double get_price() const;
  int get_order_id() const;
  int get_volume() const;
  buy_or_sell get_side() const;
  std::time_t get_timestamp() const;
  
  // Setters
  void set_client(std::string new_client);
  void set_price(double new_price);
  void set_order_id(int new_order_id);
  void set_volume(int new_volume);
  void set_side(buy_or_sell new_side);
  void set_timestamp(std::time_t new_timestamp);
};

bool operator<(const Order& a, const Order& b);
bool operator>(const Order& a, const Order& b);

#endif