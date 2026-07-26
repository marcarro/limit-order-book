#include <gtest/gtest.h>

#include "orderbook/Orderbook.h"

using namespace trading;

TEST(MatchingTests, SweepsMultiplePriceLevels) {
    Orderbook book;
    ExecutionBuffer executions{4096};
    const auto order = [](ClientId client, const char* price, OrderId id, Quantity quantity, Side side) {
        return Order{client, Price(price), id, quantity, side};
    };

    ASSERT_EQ(book.place_order(order(1, "101.0000", 1, 50, Side::SELL), executions), OrderResult::SUCCESS);
    ASSERT_EQ(book.place_order(order(2, "102.0000", 2, 30, Side::SELL), executions), OrderResult::SUCCESS);
    ASSERT_EQ(book.place_order(order(3, "100.0000", 3, 40, Side::SELL), executions), OrderResult::SUCCESS);

    EXPECT_EQ(book.place_order(order(4, "102.0000", 4, 100, Side::BUY), executions), OrderResult::COMPLETE_FILL);
    ASSERT_EQ(executions.size(), 3);
    EXPECT_EQ(executions[0].maker_order_id, 3);
    EXPECT_EQ(executions[0].quantity, 40);
    EXPECT_EQ(executions[1].maker_order_id, 1);
    EXPECT_EQ(executions[1].quantity, 50);
    EXPECT_EQ(executions[2].maker_order_id, 2);
    EXPECT_EQ(executions[2].quantity, 10);
    EXPECT_EQ(book.get_volume_at_price(Price("102.0000"), Side::SELL), 20);
}
