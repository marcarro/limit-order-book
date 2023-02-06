#include <iostream>

#include "Orderbook.h"

int main() {
  Orderbook ordbook;
  
  Order ord1("Alex G", 100, 1, 30, sell, time(0));
  Order ord2("John M", 101, 2, 30, sell, time(0));
  Order ord3("Jake T", 99, 3, 30, buy, time(0));
  Order ord4("James B", 99, 4, 30, buy, time(0));
  Order ord5("Market Maker", 98, 5, 31, sell, time(0));

  ordbook.place_order2(ord1);
  ordbook.place_order2(ord2);
  ordbook.place_order2(ord3);
  ordbook.place_order2(ord4);
  ordbook.place_order2(ord5);
  ordbook.view();

  return 0;
}