#include <gtest/gtest.h>
#include "orderbook/Orderbook.h"

using namespace trading;

class OrderbookTests : public ::testing::Test {
protected:
  Orderbook book;
  std::vector<TradeInfo> trades;
};

TEST_F(OrderbookTests, PlaceSimpleOrder) {
  auto now = std::chrono::system_clock::now();
  Order order("client1", Price("100.0000"), 1, 100, Side::BUY, now);
  
  auto result = book.place_order(order, trades);
  EXPECT_EQ(result, OrderResult::SUCCESS);
  EXPECT_TRUE(trades.empty());
  EXPECT_EQ(book.order_count(), 1);
  EXPECT_EQ(book.price_level_count(), 1);
  EXPECT_EQ(book.get_best_bid().to_double(), 100.0);
}

TEST_F(OrderbookTests, RejectInvalidOrder) {
  auto now = std::chrono::system_clock::now();
  
  // Zero volume
  Order invalid1("client1", Price("100.0000"), 1, 0, Side::BUY, now);
  auto result1 = book.place_order(invalid1, trades);
  EXPECT_EQ(result1, OrderResult::INVALID_ORDER);
  
  // Zero price
  Order invalid2("client1", Price("0.0000"), 2, 100, Side::BUY, now);
  auto result2 = book.place_order(invalid2, trades);
  EXPECT_EQ(result2, OrderResult::INVALID_ORDER);
}

TEST_F(OrderbookTests, MatchOrders) {
  auto now = std::chrono::system_clock::now();
  
  // Place sell order first
  Order sell("seller", Price("100.0000"), 1, 50, Side::SELL, now);
  auto result1 = book.place_order(sell, trades);
  EXPECT_EQ(result1, OrderResult::SUCCESS);
  EXPECT_TRUE(trades.empty());
  
  // Place matching buy order
  Order buy("buyer", Price("100.0000"), 2, 30, Side::BUY, now);
  auto result2 = book.place_order(buy, trades);
  EXPECT_EQ(result2, OrderResult::COMPLETE_FILL);
  
  // Verify trade occurred
  ASSERT_EQ(trades.size(), 1);
  EXPECT_EQ(trades[0].order_id, 2); // Buy order ID
  EXPECT_EQ(trades[0].volume, 30);
  EXPECT_EQ(trades[0].price.to_double(), 100.0);
  EXPECT_TRUE(trades[0].is_buy);
  
  // Verify remaining sell order
  EXPECT_EQ(book.get_volume_at_price(Price("100.0"), Side::SELL), 20);
}

TEST_F(OrderbookTests, CancelOrder) {
  auto now = std::chrono::system_clock::now();

  // Place orders
  Order order1("client1", Price("100.0000"), 1, 50, Side::BUY, now);
  Order order2("client2", Price("101.0000"), 2, 30, Side::BUY, now);

  book.place_order(order1, trades);
  book.place_order(order2, trades);

  EXPECT_EQ(book.order_count(), 2);
  EXPECT_EQ(book.price_level_count(), 2);

  // Cancel first order
  auto result = book.cancel_order(1);
  EXPECT_EQ(result, OrderResult::SUCCESS);
  EXPECT_EQ(book.order_count(), 1);
  EXPECT_EQ(book.price_level_count(), 1);
  EXPECT_EQ(book.get_volume_at_price(Price("100.0"), Side::BUY), 0);
  EXPECT_EQ(book.get_volume_at_price(Price("101.0"), Side::BUY), 30);

  // Cancel non-existent order
  result = book.cancel_order(999);
  EXPECT_EQ(result, OrderResult::ORDER_NOT_FOUND);

  // Cancel second order
  result = book.cancel_order(2);
  EXPECT_EQ(result, OrderResult::SUCCESS);
  EXPECT_EQ(book.order_count(), 0);
  EXPECT_EQ(book.price_level_count(), 0);
}

TEST_F(OrderbookTests, ModifyOrderVolumeOnly) {
  auto now = std::chrono::system_clock::now();

  // Place order
  Order order("client1", Price("100.0000"), 1, 100, Side::BUY, now);
  book.place_order(order, trades);

  EXPECT_EQ(book.get_volume_at_price(Price("100.0"), Side::BUY), 100);

  // Reduce volume at same price
  auto result = book.modify_order(1, Price("100.0000"), 50);
  EXPECT_EQ(result, OrderResult::SUCCESS);
  EXPECT_EQ(book.get_volume_at_price(Price("100.0"), Side::BUY), 50);
  EXPECT_EQ(book.order_count(), 1);
}

TEST_F(OrderbookTests, ModifyOrderPrice) {
  auto now = std::chrono::system_clock::now();

  // Place order
  Order order("client1", Price("100.0000"), 1, 100, Side::BUY, now);
  book.place_order(order, trades);

  // Modify price (should cancel and re-place)
  auto result = book.modify_order(1, Price("101.0000"), 100);
  EXPECT_EQ(result, OrderResult::SUCCESS);
  EXPECT_EQ(book.get_volume_at_price(Price("100.0"), Side::BUY), 0);
  EXPECT_EQ(book.get_volume_at_price(Price("101.0"), Side::BUY), 100);
  EXPECT_EQ(book.order_count(), 1);
  EXPECT_EQ(book.price_level_count(), 1);
}

TEST_F(OrderbookTests, ModifyOrderNonExistent) {
  auto result = book.modify_order(999, Price("100.0000"), 50);
  EXPECT_EQ(result, OrderResult::ORDER_NOT_FOUND);
}

TEST_F(OrderbookTests, DuplicateOrderID) {
  auto now = std::chrono::system_clock::now();

  // Place first order
  Order order1("client1", Price("100.0000"), 1, 50, Side::BUY, now);
  auto result1 = book.place_order(order1, trades);
  EXPECT_EQ(result1, OrderResult::SUCCESS);

  // Try to place order with same ID
  Order order2("client2", Price("101.0000"), 1, 30, Side::BUY, now);
  auto result2 = book.place_order(order2, trades);
  EXPECT_EQ(result2, OrderResult::DUPLICATE_ORDER_ID);
  EXPECT_EQ(book.order_count(), 1);
}

TEST_F(OrderbookTests, SellMatchingAgainstBids) {
  auto now = std::chrono::system_clock::now();

  // Place buy orders
  Order buy1("buyer1", Price("100.0000"), 1, 50, Side::BUY, now);
  Order buy2("buyer2", Price("99.0000"), 2, 30, Side::BUY, now);

  book.place_order(buy1, trades);
  book.place_order(buy2, trades);
  EXPECT_TRUE(trades.empty());

  // Place matching sell order
  Order sell("seller", Price("99.0000"), 3, 40, Side::SELL, now);
  auto result = book.place_order(sell, trades);
  EXPECT_EQ(result, OrderResult::COMPLETE_FILL);

  // Should match against best bid (100.0) first
  ASSERT_EQ(trades.size(), 1);
  EXPECT_EQ(trades[0].order_id, 3);
  EXPECT_EQ(trades[0].volume, 40);
  EXPECT_EQ(trades[0].price.to_double(), 100.0);
  EXPECT_FALSE(trades[0].is_buy);
  EXPECT_EQ(trades[0].counterparty, "buyer1");

  // Verify remaining buy order
  EXPECT_EQ(book.get_volume_at_price(Price("100.0"), Side::BUY), 10);
}

TEST_F(OrderbookTests, PriceTimePriority) {
  auto now = std::chrono::system_clock::now();

  // Place three sell orders at same price, different times
  Order order1("client1", Price("100.0000"), 1, 50, Side::SELL, now);
  Order order2("client2", Price("100.0000"), 2, 30, Side::SELL, now);
  Order order3("client3", Price("100.0000"), 3, 20, Side::SELL, now);

  book.place_order(order1, trades);
  book.place_order(order2, trades);
  book.place_order(order3, trades);

  EXPECT_EQ(book.get_volume_at_price(Price("100.0"), Side::SELL), 100);

  // Clear trades from placing sell orders
  trades.clear();

  // Place buy at higher price to ensure matching
  Order buy("buyer", Price("101.0000"), 4, 60, Side::BUY, now);
  auto result = book.place_order(buy, trades);
  EXPECT_EQ(result, OrderResult::COMPLETE_FILL);

  // Should match in time priority: order1 (50), then order2 (10)
  ASSERT_EQ(trades.size(), 2);
  EXPECT_EQ(trades[0].counterparty, "client1");
  EXPECT_EQ(trades[0].volume, 50);
  EXPECT_EQ(trades[1].counterparty, "client2");
  EXPECT_EQ(trades[1].volume, 10);

  // Verify remaining orders (order2 has 20 left, order3 has 20)
  EXPECT_EQ(book.order_count(), 2);
  EXPECT_EQ(book.get_volume_at_price(Price("100.0"), Side::SELL), 40);
}

TEST_F(OrderbookTests, EmptyBookQueries) {
  // Query empty book
  EXPECT_EQ(book.get_best_bid().to_double(), 0.0);
  EXPECT_EQ(book.get_best_ask().to_double(), 0.0);
  EXPECT_EQ(book.get_mid_price().to_double(), 0.0);
  EXPECT_EQ(book.order_count(), 0);
  EXPECT_EQ(book.price_level_count(), 0);

  auto bids = book.get_bid_levels(5);
  auto asks = book.get_ask_levels(5);
  EXPECT_TRUE(bids.empty());
  EXPECT_TRUE(asks.empty());
}

TEST_F(OrderbookTests, MidPrice) {
  auto now = std::chrono::system_clock::now();

  // Place bid and ask
  Order bid("buyer", Price("100.0000"), 1, 50, Side::BUY, now);
  Order ask("seller", Price("102.0000"), 2, 50, Side::SELL, now);

  book.place_order(bid, trades);
  book.place_order(ask, trades);

  // Mid price should be (100 + 102) / 2 = 101
  EXPECT_EQ(book.get_mid_price().to_double(), 101.0);
}

TEST_F(OrderbookTests, MarketDataDepth) {
  auto now = std::chrono::system_clock::now();

  // Place multiple levels
  book.place_order(Order("b1", Price("100.0000"), 1, 50, Side::BUY, now), trades);
  book.place_order(Order("b2", Price("99.0000"), 2, 30, Side::BUY, now), trades);
  book.place_order(Order("b3", Price("98.0000"), 3, 20, Side::BUY, now), trades);

  book.place_order(Order("s1", Price("101.0000"), 4, 40, Side::SELL, now), trades);
  book.place_order(Order("s2", Price("102.0000"), 5, 35, Side::SELL, now), trades);
  book.place_order(Order("s3", Price("103.0000"), 6, 25, Side::SELL, now), trades);

  // Get bid levels
  auto bids = book.get_bid_levels(3);
  ASSERT_EQ(bids.size(), 3);
  EXPECT_EQ(bids[0].price.to_double(), 100.0);
  EXPECT_EQ(bids[0].total_volume, 50);
  EXPECT_EQ(bids[0].order_count, 1);
  EXPECT_EQ(bids[1].price.to_double(), 99.0);
  EXPECT_EQ(bids[2].price.to_double(), 98.0);

  // Get ask levels
  auto asks = book.get_ask_levels(3);
  ASSERT_EQ(asks.size(), 3);
  EXPECT_EQ(asks[0].price.to_double(), 101.0);
  EXPECT_EQ(asks[0].total_volume, 40);
  EXPECT_EQ(asks[0].order_count, 1);
  EXPECT_EQ(asks[1].price.to_double(), 102.0);
  EXPECT_EQ(asks[2].price.to_double(), 103.0);
}