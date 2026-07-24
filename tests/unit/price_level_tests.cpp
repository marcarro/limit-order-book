#include <gtest/gtest.h>

#include "orderbook/PriceLevel.h"

using namespace trading;

TEST(PriceLevelTests, MaintainsFifoAndVolume) {
    PriceLevel level;
    level.reset(Price("100.0000"));
    Order first{1, Price("100.0000"), 1, 50, Side::BUY};
    Order second{2, Price("100.0000"), 2, 30, Side::BUY};

    level.add_order(&first);
    level.add_order(&second);

    EXPECT_EQ(level.order_count, 2);
    EXPECT_EQ(level.total_volume, 80);
    EXPECT_EQ(level.head, &first);
    EXPECT_EQ(level.tail, &second);
    EXPECT_EQ(first.next, &second);
    EXPECT_EQ(second.prev, &first);

    level.remove_order(&first);
    EXPECT_EQ(level.order_count, 1);
    EXPECT_EQ(level.total_volume, 30);
    EXPECT_EQ(level.head, &second);
    EXPECT_EQ(second.prev, nullptr);
}
