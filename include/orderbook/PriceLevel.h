#pragma once

#include <cstddef>
#include <vector>

#include "Order.h"
#include "../common/FixedHashMap.h"
#include "../common/FixedPool.h"

namespace trading {

class PriceLevel {
public:
    Price price{};
    Quantity total_volume = 0;
    std::size_t order_count = 0;
    std::size_t heap_index = 0;
    Order* head = nullptr;
    Order* tail = nullptr;

    void reset(const Price& new_price);
    void add_order(Order* order);
    void remove_order(Order* order);
    void update_quantity(Order* order, Quantity old_quantity);
};

class PriceLevelSide {
public:
    PriceLevelSide(bool bid_side, std::size_t max_price_levels);

    PriceLevel* find(const Price& price);
    const PriceLevel* find(const Price& price) const;
    PriceLevel* best();
    const PriceLevel* best() const;
    bool empty() const { return heap_size_ == 0; }
    std::size_t size() const { return heap_size_; }
    bool full() const { return levels_by_price_.full(); }

    PriceLevel* create(const Price& price, memory::FixedPool<PriceLevel>& pool);
    void remove(PriceLevel* level, memory::FixedPool<PriceLevel>& pool);
    void collect(std::vector<const PriceLevel*>& levels) const;

    template <typename Fn>
    void for_each(Fn&& fn) const {
        for (std::size_t index = 0; index < heap_size_; ++index) {
            fn(*heap_[index]);
        }
    }

private:
    bool better(const PriceLevel* lhs, const PriceLevel* rhs) const;
    void sift_up(std::size_t index);
    void sift_down(std::size_t index);
    void remove_from_heap(std::size_t index);

    bool bid_side_;
    memory::FixedHashMap<std::int64_t, PriceLevel*> levels_by_price_;
    std::vector<PriceLevel*> heap_;
    std::size_t heap_size_ = 0;
};

} // namespace trading
