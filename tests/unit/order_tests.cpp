#include <gtest/gtest.h>

#include "orderbook/Order.h"

using namespace trading;

TEST(OrderTests, IsCompactNumericData) {
    Order order{42, Price("150.5000"), 1234, 100, Side::BUY};

    EXPECT_EQ(order.client_id, 42);
    EXPECT_EQ(order.price.to_double(), 150.5);
    EXPECT_EQ(order.order_id, 1234);
    EXPECT_EQ(order.quantity, 100);
    EXPECT_EQ(order.side, Side::BUY);
    EXPECT_EQ(order.next, nullptr);
    EXPECT_EQ(order.prev, nullptr);
    EXPECT_EQ(order.level, nullptr);
}
