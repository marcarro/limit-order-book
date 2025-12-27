#pragma once
#include "Order.h"
#include "../common/FixedPoint.h"
#include "../common/MemoryPool.h"
#include <unordered_map>

namespace trading {

class PriceLevel {
private:
    Price price_;
    int total_volume_ = 0;
    int order_count_ = 0;

public:
    // intrusive order list
    Order* head = nullptr;
    Order* tail = nullptr;
    
    // intrusive price level list
    PriceLevel* next_price = nullptr;
    PriceLevel* prev_price = nullptr;
    
    // constructors
    explicit PriceLevel(const Price& price);
    
    // accessors
    Price get_price() const;
    int get_total_volume() const;
    int get_order_count() const;
    
    // order management
    void add_order(Order* order);
    void remove_order(Order* order);
    void update_volume(Order* order, int old_volume);
};

// Manages a linked list of price levels
class PriceLevelList {
private:
    PriceLevel* head_ = nullptr;
    PriceLevel* tail_ = nullptr;
    bool is_bid_side_;
	memory::MemoryPool<PriceLevel>& pool_;
    
    // fast lookup by price
    std::unordered_map<Price, PriceLevel*, std::hash<Price>> price_map_;

public:
	explicit PriceLevelList(bool is_bid_side, memory::MemoryPool<PriceLevel>& pool)
        : head_(nullptr), tail_(nullptr), is_bid_side_(is_bid_side), pool_(pool) {}

    
    PriceLevel* find_level(const Price& price) const;
    PriceLevel* create_level(const Price& price);
    void remove_level(PriceLevel* level);
    
    PriceLevel* get_best_level() const;
    bool empty() const;
    
    // iterate through price levels
    PriceLevel* begin() const;
    PriceLevel* next(PriceLevel* current) const;
};

}
