#include <iostream>

#include "Orderbook.h"

int main() {
  Order ord1("Alex G", 100, 1, 30, sell, time(0));
  Order ord2("John M", 100, 2, 15, buy, time(0));

  Orderbook ordbook;
  ordbook.place_order(ord1);
  ordbook.place_order(ord2);
  std::cout << "ord1 addr in ordbook: " << &ordbook.asks[100].front() << std::endl;
  return 0;
}