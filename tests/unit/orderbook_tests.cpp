#include <gtest/gtest.h>

#include "orderbook/Orderbook.h"

using namespace trading;

namespace {

Order make_order(ClientId client, const char* price, OrderId id, Quantity quantity, Side side) {
    return Order{client, Price(price), id, quantity, side};
}

} // namespace

class OrderbookTests : public ::testing::Test {
protected:
    Orderbook book;
    ExecutionBuffer executions{4096};
};

TEST_F(OrderbookTests, PlacesAndCancelsRestingOrders) {
    EXPECT_EQ(book.place_order(make_order(1, "100.0000", 1, 100, Side::BUY), executions), OrderResult::SUCCESS);
    EXPECT_EQ(book.order_count(), 1);
    EXPECT_EQ(book.price_level_count(), 1);
    EXPECT_EQ(book.get_best_bid().to_double(), 100.0);

    EXPECT_EQ(book.cancel_order(1), OrderResult::SUCCESS);
    EXPECT_EQ(book.order_count(), 0);
    EXPECT_EQ(book.price_level_count(), 0);
    EXPECT_EQ(book.cancel_order(1), OrderResult::ORDER_NOT_FOUND);
}

TEST_F(OrderbookTests, RejectsInvalidDuplicateAndUndersizedOutput) {
    EXPECT_EQ(book.place_order(make_order(1, "100.0000", 0, 10, Side::BUY), executions), OrderResult::INVALID_ORDER);
    EXPECT_EQ(book.place_order(make_order(1, "100.0000", 1, 0, Side::BUY), executions), OrderResult::INVALID_ORDER);

    ExecutionBuffer undersized{8};
    EXPECT_EQ(book.place_order(make_order(1, "100.0000", 1, 10, Side::BUY), undersized), OrderResult::EXECUTION_BUFFER_TOO_SMALL);
    EXPECT_EQ(book.order_count(), 0);

    EXPECT_EQ(book.place_order(make_order(1, "100.0000", 1, 10, Side::BUY), executions), OrderResult::SUCCESS);
    EXPECT_EQ(book.place_order(make_order(2, "101.0000", 1, 10, Side::BUY), executions), OrderResult::DUPLICATE_ORDER_ID);
}

TEST_F(OrderbookTests, MatchesBestPriceAndEmitsNumericExecution) {
    ASSERT_EQ(book.place_order(make_order(10, "100.0000", 1, 50, Side::BUY), executions), OrderResult::SUCCESS);
    ASSERT_EQ(book.place_order(make_order(20, "99.0000", 2, 30, Side::BUY), executions), OrderResult::SUCCESS);

    EXPECT_EQ(book.place_order(make_order(30, "99.0000", 3, 40, Side::SELL), executions), OrderResult::COMPLETE_FILL);
    ASSERT_EQ(executions.size(), 1);
    EXPECT_EQ(executions[0].taker_order_id, 3);
    EXPECT_EQ(executions[0].maker_order_id, 1);
    EXPECT_EQ(executions[0].taker_client_id, 30);
    EXPECT_EQ(executions[0].maker_client_id, 10);
    EXPECT_EQ(executions[0].quantity, 40);
    EXPECT_EQ(executions[0].price.to_double(), 100.0);
    EXPECT_EQ(book.get_volume_at_price(Price("100.0000"), Side::BUY), 10);
}

TEST_F(OrderbookTests, PreservesPriceTimePriority) {
    ASSERT_EQ(book.place_order(make_order(1, "100.0000", 1, 50, Side::SELL), executions), OrderResult::SUCCESS);
    ASSERT_EQ(book.place_order(make_order(2, "100.0000", 2, 30, Side::SELL), executions), OrderResult::SUCCESS);
    ASSERT_EQ(book.place_order(make_order(3, "100.0000", 3, 20, Side::SELL), executions), OrderResult::SUCCESS);

    EXPECT_EQ(book.place_order(make_order(4, "101.0000", 4, 60, Side::BUY), executions), OrderResult::COMPLETE_FILL);
    ASSERT_EQ(executions.size(), 2);
    EXPECT_EQ(executions[0].maker_order_id, 1);
    EXPECT_EQ(executions[0].quantity, 50);
    EXPECT_EQ(executions[1].maker_order_id, 2);
    EXPECT_EQ(executions[1].quantity, 10);
}

TEST_F(OrderbookTests, ModifiesOrdersWithoutAllocatingOutput) {
    ASSERT_EQ(book.place_order(make_order(1, "100.0000", 1, 100, Side::BUY), executions), OrderResult::SUCCESS);
    EXPECT_EQ(book.modify_order(1, Price("100.0000"), 50, executions), OrderResult::SUCCESS);
    EXPECT_EQ(executions.size(), 0);
    EXPECT_EQ(book.get_volume_at_price(Price("100.0000"), Side::BUY), 50);

    EXPECT_EQ(book.modify_order(1, Price("101.0000"), 50, executions), OrderResult::SUCCESS);
    EXPECT_EQ(book.get_volume_at_price(Price("100.0000"), Side::BUY), 0);
    EXPECT_EQ(book.get_volume_at_price(Price("101.0000"), Side::BUY), 50);
}

TEST_F(OrderbookTests, ReturnsMarketDepthInPriceOrder) {
    ASSERT_EQ(book.place_order(make_order(1, "100.0000", 1, 50, Side::BUY), executions), OrderResult::SUCCESS);
    ASSERT_EQ(book.place_order(make_order(2, "99.0000", 2, 30, Side::BUY), executions), OrderResult::SUCCESS);
    ASSERT_EQ(book.place_order(make_order(3, "101.0000", 3, 40, Side::SELL), executions), OrderResult::SUCCESS);
    ASSERT_EQ(book.place_order(make_order(4, "102.0000", 4, 35, Side::SELL), executions), OrderResult::SUCCESS);

    const auto bids = book.get_bid_levels();
    const auto asks = book.get_ask_levels();
    ASSERT_EQ(bids.size(), 2);
    ASSERT_EQ(asks.size(), 2);
    EXPECT_EQ(bids[0].price.to_double(), 100.0);
    EXPECT_EQ(bids[1].price.to_double(), 99.0);
    EXPECT_EQ(asks[0].price.to_double(), 101.0);
    EXPECT_EQ(asks[1].price.to_double(), 102.0);
}

TEST(OrderbookCapacityTests, RejectsRestingOrderWhenFullWithoutMutatingBook) {
    Orderbook book(OrderbookConfig{1, 1});
    ExecutionBuffer executions{1};
    ASSERT_EQ(book.place_order(make_order(1, "100.0000", 1, 10, Side::BUY), executions), OrderResult::SUCCESS);
    EXPECT_EQ(book.place_order(make_order(2, "99.0000", 2, 10, Side::BUY), executions), OrderResult::BOOK_FULL);
    EXPECT_EQ(book.order_count(), 1);
    EXPECT_EQ(book.get_best_bid().to_double(), 100.0);
}

TEST(OrderbookCapacityTests, AcceptsAFullMatchWhenNoRestingSlotRemains) {
    Orderbook book(OrderbookConfig{1, 1});
    ExecutionBuffer executions{1};
    ASSERT_EQ(book.place_order(make_order(1, "100.0000", 1, 10, Side::SELL), executions), OrderResult::SUCCESS);

    EXPECT_EQ(book.place_order(make_order(2, "100.0000", 2, 10, Side::BUY), executions), OrderResult::COMPLETE_FILL);
    ASSERT_EQ(executions.size(), 1);
    EXPECT_EQ(executions[0].maker_order_id, 1);
    EXPECT_EQ(book.order_count(), 0);
}

TEST(OrderbookCapacityTests, RejectsNewPriceLevelWhenTheSharedPoolIsFull) {
    Orderbook book(OrderbookConfig{4, 1});
    ExecutionBuffer executions{4};
    ASSERT_EQ(book.place_order(make_order(1, "100.0000", 1, 10, Side::BUY), executions), OrderResult::SUCCESS);

    EXPECT_EQ(book.place_order(make_order(2, "102.0000", 2, 10, Side::SELL), executions), OrderResult::PRICE_LEVEL_LIMIT);
    EXPECT_EQ(book.order_count(), 1);
    EXPECT_EQ(book.price_level_count(), 1);
    EXPECT_EQ(book.get_best_bid().to_double(), 100.0);
}

TEST(OrderbookCapacityTests, DoesNotDropAnOrderWhenModifyNeedsAFullSharedPriceLevelPool) {
    Orderbook book(OrderbookConfig{4, 2});
    ExecutionBuffer executions{4};
    ASSERT_EQ(book.place_order(make_order(1, "100.0000", 1, 10, Side::BUY), executions), OrderResult::SUCCESS);
    ASSERT_EQ(book.place_order(make_order(2, "100.0000", 2, 10, Side::BUY), executions), OrderResult::SUCCESS);
    ASSERT_EQ(book.place_order(make_order(3, "102.0000", 3, 10, Side::SELL), executions), OrderResult::SUCCESS);

    EXPECT_EQ(book.modify_order(1, Price("99.0000"), 10, executions), OrderResult::PRICE_LEVEL_LIMIT);
    EXPECT_EQ(book.order_count(), 3);
    EXPECT_EQ(book.get_volume_at_price(Price("100.0000"), Side::BUY), 20);
    EXPECT_EQ(book.get_volume_at_price(Price("99.0000"), Side::BUY), 0);
}
