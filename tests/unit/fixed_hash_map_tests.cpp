#include <gtest/gtest.h>

#include "common/FixedHashMap.h"

namespace {

struct CollisionKey {
    int value = 0;
    bool operator==(const CollisionKey& other) const { return value == other.value; }
};

} // namespace

namespace std {

template <>
struct hash<CollisionKey> {
    std::size_t operator()(const CollisionKey&) const { return 0; }
};

} // namespace std

TEST(FixedHashMapTests, BackshiftDeletionPreservesCollisionChain) {
    trading::memory::FixedHashMap<CollisionKey, int> map(3);
    ASSERT_TRUE(map.insert(CollisionKey{1}, 10));
    ASSERT_TRUE(map.insert(CollisionKey{2}, 20));
    ASSERT_TRUE(map.insert(CollisionKey{3}, 30));

    ASSERT_TRUE(map.erase(CollisionKey{2}));
    ASSERT_NE(map.find(CollisionKey{1}), nullptr);
    ASSERT_NE(map.find(CollisionKey{3}), nullptr);
    EXPECT_EQ(*map.find(CollisionKey{1}), 10);
    EXPECT_EQ(*map.find(CollisionKey{3}), 30);
    EXPECT_TRUE(map.insert(CollisionKey{4}, 40));
}
