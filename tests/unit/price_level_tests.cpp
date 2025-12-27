#include <gtest/gtest.h>
#include "orderbook/PriceLevel.h"
#include "common/MemoryPool.h"

using namespace trading;

class PriceLevelTests : public ::testing::Test {
protected:
  void SetUp() override {
    level = new PriceLevel(Price("100.0000"));
  }
  
  void TearDown() override {
    delete level;
  }
  
  PriceLevel* level;
};

TEST_F(PriceLevelTests, Construction) {
  EXPECT_EQ(level->get_price().to_double(), 100.0);
  EXPECT_EQ(level->get_total_volume(), 0);
  EXPECT_EQ(level->get_order_count(), 0);
  EXPECT_EQ(level->head, nullptr);
  EXPECT_EQ(level->tail, nullptr);
}

TEST_F(PriceLevelTests, AddOrder) {
  // Create orders manually for test
  auto now = std::chrono::system_clock::now();
  Order* order1 = new Order("client1", Price("100.0000"), 1, 50, Side::BUY, now);
  Order* order2 = new Order("client2", Price("100.0000"), 2, 30, Side::BUY, now);
  
  // Add first order
  level->add_order(order1);
  EXPECT_EQ(level->get_order_count(), 1);
  EXPECT_EQ(level->get_total_volume(), 50);
  EXPECT_EQ(level->head, order1);
  EXPECT_EQ(level->tail, order1);
  EXPECT_EQ(order1->level, level);
  
  // Add second order
  level->add_order(order2);
  EXPECT_EQ(level->get_order_count(), 2);
  EXPECT_EQ(level->get_total_volume(), 80);
  EXPECT_EQ(level->head, order1);
  EXPECT_EQ(level->tail, order2);
  EXPECT_EQ(order1->next, order2);
  EXPECT_EQ(order2->prev, order1);
  
  // Clean up
  delete order1;
  delete order2;
}

TEST_F(PriceLevelTests, RemoveOrder) {
  // Setup 3 orders
  auto now = std::chrono::system_clock::now();
  Order* order1 = new Order("client1", Price("100.0000"), 1, 50, Side::BUY, now);
  Order* order2 = new Order("client2", Price("100.0000"), 2, 30, Side::BUY, now);
  Order* order3 = new Order("client3", Price("100.0000"), 3, 20, Side::BUY, now);
  
  level->add_order(order1);
  level->add_order(order2);
  level->add_order(order3);
  
  // Remove middle order
  level->remove_order(order2);
  EXPECT_EQ(level->get_order_count(), 2);
  EXPECT_EQ(level->get_total_volume(), 70);
  EXPECT_EQ(order1->next, order3);
  EXPECT_EQ(order3->prev, order1);
  EXPECT_EQ(order2->level, nullptr);
  
  // Clean up
  delete order1;
  delete order2;
  delete order3;
}