#include <gtest/gtest.h>
#include "orderbook/Order.h"

using namespace trading;

TEST(OrderTests, BasicConstruction) {
  auto now = std::chrono::system_clock::now();
  
  Order order("client1", Price("150.5000"), 1234, 100, Side::BUY, now);
  
  EXPECT_EQ(order.get_client(), "client1");
  EXPECT_EQ(order.get_price().to_double(), 150.5);
  EXPECT_EQ(order.get_order_id(), 1234);
  EXPECT_EQ(order.get_volume(), 100);
  EXPECT_EQ(order.get_side(), Side::BUY);
  EXPECT_EQ(order.get_timestamp(), now);
  
  // verify default links are null
  EXPECT_EQ(order.next, nullptr);
  EXPECT_EQ(order.prev, nullptr);
  EXPECT_EQ(order.level, nullptr);
}

TEST(OrderTests, Setters) {
  Order order;
  
  order.set_client("newclient");
  EXPECT_EQ(order.get_client(), "newclient");
  
  order.set_price(Price("42.0000"));
  EXPECT_EQ(order.get_price().to_double(), 42.0);
  
  order.set_order_id(5678);
  EXPECT_EQ(order.get_order_id(), 5678);
  
  order.set_volume(200);
  EXPECT_EQ(order.get_volume(), 200);
  
  order.set_side(Side::SELL);
  EXPECT_EQ(order.get_side(), Side::SELL);
  
  auto new_time = std::chrono::system_clock::now();
  order.set_timestamp(new_time);
  EXPECT_EQ(order.get_timestamp(), new_time);
}