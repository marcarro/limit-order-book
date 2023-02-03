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
  std::string client;
  double price;
  int order_id;
  int volume;
  buy_or_sell side;
  std::time_t timestamp;
public:
  
  // Getters
  std::string get_client();
  double get_price();
  int get_order_id();
  int get_volume();
  buy_or_sell get_side();
  std::time_t get_timestamp();
  
  // Setters
  void set_client(std::string new_client);
  void set_price(double new_price);
  void set_order_id(int new_order_id);
  void set_volume(int new_volume);
  void set_side(buy_or_sell new_side);
  void set_timestamp(std::time_t new_timestamp);
};

#endif