#pragma once

#include "OrderbookTypes.h"

namespace trading {

class PriceLevel;

struct Order {
    ClientId client_id = 0;
    Price price{};
    OrderId order_id = 0;
    Quantity quantity = 0;
    Side side = Side::BUY;

    Order* next = nullptr;
    Order* prev = nullptr;
    PriceLevel* level = nullptr;
};

} // namespace trading
