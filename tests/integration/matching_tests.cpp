#include <gtest/gtest.h>
#include "orderbook/Orderbook.h"

using namespace trading;

class MatchingTests : public ::testing::Test {
protected:
  Orderbook book;
  std::vector<TradeInfo> trades;
  
  void SetUp() override {
    trades.clear();
  }
};

TEST_F(MatchingTests, ComplexOrderMatching) {
  auto now = std::chrono::system_clock::now();
  
  // Set up a book with multiple orders
  Order sell1("seller1", Price("101.0000"), 1, 50, Side::SELL, now);
  Order sell2("seller2", Price("102.0000"), 2, 30, Side::SELL, now);
  Order sell3("seller3", Price("100.0000"), 3, 40, Side::SELL, now);
  
  book.place_order(sell1, trades);
  book.place_order(sell2, trades);
  book.place_order(sell3, trades);
  EXPECT_TRUE(trades.empty());
  
  // Best ask should be 100.0
  EXPECT_EQ(book.get_best_ask().to_double(), 100.0);
  
  // Place a large buy order that will match against multiple levels
  Order buy("buyer1", Price("102.0000"), 4, 100, Side::BUY, now);
  auto result = book.place_order(buy, trades);

  // Should be completely filled
  EXPECT_EQ(result, OrderResult::COMPLETE_FILL);
  
  // Should have executed 3 trades (one for each sell order)
  ASSERT_EQ(trades.size(), 3);
  
  // First trade should be against the best ask (sell3)
  EXPECT_EQ(trades[0].counterparty, "seller3");
  EXPECT_EQ(trades[0].volume, 40);
  EXPECT_EQ(trades[0].price.to_double(), 100.0);
  
  // Second trade should be against the next best ask (sell1)
  EXPECT_EQ(trades[1].counterparty, "seller1");
  EXPECT_EQ(trades[1].volume, 50);
  EXPECT_EQ(trades[1].price.to_double(), 101.0);
  
  // Third trade should be against the last ask (sell2)
  EXPECT_EQ(trades[2].counterparty, "seller2");
  EXPECT_EQ(trades[2].volume, 10); // Only 10 of the 30 available
  EXPECT_EQ(trades[2].price.to_double(), 102.0);
  
  // Should be 20 remaining volume at 102.0
  EXPECT_EQ(book.get_volume_at_price(Price("102.0"), Side::SELL), 20);
  
  // Order book should have 1 price level with 1 order
  EXPECT_EQ(book.price_level_count(), 1);
  EXPECT_EQ(book.order_count(), 1);
}