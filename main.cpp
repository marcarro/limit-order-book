#include <iostream>

#include "Orderbook.h"
#include "Benchmark.h"

int main() {
  Orderbook ordbook;
  
  {
    Benchmark benchmark;
    Order ord1("Alex G", 100, 1, 30, sell, time(0));
    ordbook.place_order(ord1);
  }

  {
    Benchmark benchmark;
    Order ord2("John M", 101, 2, 30, sell, time(0));
    ordbook.place_order(ord2);
  }

  {
    Benchmark benchmark;
    Order ord3("Jake T", 99, 3, 30, buy, time(0));
    ordbook.place_order(ord3);
  }
    
  {
    Benchmark benchmark;
    Order ord4("James B", 99, 4, 30, buy, time(0));
    ordbook.place_order(ord4);
  }
  
  {
    Benchmark benchmark;
    Order ord5("Market Maker", 102, 5, 32, buy, time(0));
    ordbook.place_order(ord5);
  }

  return 0;
}