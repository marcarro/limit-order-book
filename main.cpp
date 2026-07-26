#include <iostream>

#include "orderbook/Orderbook.h"

using namespace trading;

int main() {
    Orderbook book;
    ExecutionBuffer executions{book.max_orders()};

    book.place_order(Order{1, Price("99.9500"), 1, 200, Side::BUY}, executions);
    book.place_order(Order{2, Price("99.9000"), 2, 150, Side::BUY}, executions);
    book.place_order(Order{3, Price("100.0500"), 3, 100, Side::SELL}, executions);
    book.place_order(Order{4, Price("100.1000"), 4, 250, Side::SELL}, executions);

    std::cout << "orders: " << book.order_count() << '\n';
    std::cout << "best bid: " << book.get_best_bid() << '\n';
    std::cout << "best ask: " << book.get_best_ask() << '\n';
    book.print_book();
    return 0;
}
