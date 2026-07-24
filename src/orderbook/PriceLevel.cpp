#include "orderbook/PriceLevel.h"

#include <algorithm>
#include <cassert>

namespace trading {

void PriceLevel::reset(const Price& new_price) {
    price = new_price;
    total_volume = 0;
    order_count = 0;
    heap_index = 0;
    head = nullptr;
    tail = nullptr;
}

void PriceLevel::add_order(Order* order) {
    order->level = this;
    order->prev = tail;
    order->next = nullptr;
    if (tail) {
        tail->next = order;
    } else {
        head = order;
    }
    tail = order;
    total_volume += order->quantity;
    ++order_count;
}

void PriceLevel::remove_order(Order* order) {
    assert(order->level == this);
    if (order->prev) {
        order->prev->next = order->next;
    } else {
        head = order->next;
    }
    if (order->next) {
        order->next->prev = order->prev;
    } else {
        tail = order->prev;
    }
    total_volume -= order->quantity;
    --order_count;
    order->next = nullptr;
    order->prev = nullptr;
    order->level = nullptr;
}

void PriceLevel::update_quantity(Order* order, Quantity old_quantity) {
    assert(order->level == this);
    total_volume += order->quantity - old_quantity;
}

PriceLevelSide::PriceLevelSide(bool bid_side, std::size_t max_price_levels)
    : bid_side_(bid_side), levels_by_price_(max_price_levels), heap_(max_price_levels) {}

PriceLevel* PriceLevelSide::find(const Price& price) {
    PriceLevel* const* level = levels_by_price_.find(price.raw_value());
    return level ? *level : nullptr;
}

const PriceLevel* PriceLevelSide::find(const Price& price) const {
    PriceLevel* const* level = levels_by_price_.find(price.raw_value());
    return level ? *level : nullptr;
}

PriceLevel* PriceLevelSide::best() {
    return heap_size_ == 0 ? nullptr : heap_[0];
}

const PriceLevel* PriceLevelSide::best() const {
    return heap_size_ == 0 ? nullptr : heap_[0];
}

PriceLevel* PriceLevelSide::create(const Price& price, memory::FixedPool<PriceLevel>& pool) {
    if (PriceLevel* existing = find(price)) {
        return existing;
    }
    if (full()) {
        return nullptr;
    }

    PriceLevel* level = pool.acquire();
    if (!level) {
        return nullptr;
    }
    level->reset(price);
    const bool inserted = levels_by_price_.insert(price.raw_value(), level);
    assert(inserted);
    (void)inserted;
    heap_[heap_size_] = level;
    level->heap_index = heap_size_;
    sift_up(heap_size_++);
    return level;
}

void PriceLevelSide::remove(PriceLevel* level, memory::FixedPool<PriceLevel>& pool) {
    assert(level && level->order_count == 0);
    const bool removed = levels_by_price_.erase(level->price.raw_value());
    assert(removed);
    (void)removed;
    remove_from_heap(level->heap_index);
    pool.release(level);
}

void PriceLevelSide::collect(std::vector<const PriceLevel*>& levels) const {
    levels.reserve(levels.size() + heap_size_);
    for (std::size_t index = 0; index < heap_size_; ++index) {
        levels.push_back(heap_[index]);
    }
}

bool PriceLevelSide::better(const PriceLevel* lhs, const PriceLevel* rhs) const {
    return bid_side_ ? lhs->price > rhs->price : lhs->price < rhs->price;
}

void PriceLevelSide::sift_up(std::size_t index) {
    while (index > 0) {
        const std::size_t parent = (index - 1) / 2;
        if (!better(heap_[index], heap_[parent])) {
            break;
        }
        std::swap(heap_[index], heap_[parent]);
        heap_[index]->heap_index = index;
        heap_[parent]->heap_index = parent;
        index = parent;
    }
}

void PriceLevelSide::sift_down(std::size_t index) {
    while (true) {
        const std::size_t left = (index * 2) + 1;
        if (left >= heap_size_) {
            return;
        }
        const std::size_t right = left + 1;
        std::size_t best_child = left;
        if (right < heap_size_ && better(heap_[right], heap_[left])) {
            best_child = right;
        }
        if (!better(heap_[best_child], heap_[index])) {
            return;
        }
        std::swap(heap_[index], heap_[best_child]);
        heap_[index]->heap_index = index;
        heap_[best_child]->heap_index = best_child;
        index = best_child;
    }
}

void PriceLevelSide::remove_from_heap(std::size_t index) {
    assert(index < heap_size_);
    --heap_size_;
    if (index == heap_size_) {
        return;
    }
    heap_[index] = heap_[heap_size_];
    heap_[index]->heap_index = index;
    if (index > 0 && better(heap_[index], heap_[(index - 1) / 2])) {
        sift_up(index);
    } else {
        sift_down(index);
    }
}

} // namespace trading
