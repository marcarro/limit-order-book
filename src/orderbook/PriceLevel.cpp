#include "../../include/orderbook/PriceLevel.h"

namespace trading {

PriceLevel::PriceLevel(const Price& price) :
    price_(price),
    total_volume_(0),
    order_count_(0),
    head(nullptr),
    tail(nullptr),
    next_price(nullptr),
    prev_price(nullptr)
{}

Price PriceLevel::get_price() const {
    return price_;
}

int PriceLevel::get_total_volume() const {
    return total_volume_;
}

int PriceLevel::get_order_count() const {
    return order_count_;
}

void PriceLevel::add_order(Order* order) {
    // Set back-pointer to this level
    order->level = this;
    
    // Add order to the end of the list for this price level
    if (!head) {
        // First order at this price
        head = tail = order;
        order->prev = nullptr;
        order->next = nullptr;
    } else {
        // Append to existing orders
        tail->next = order;
        order->prev = tail;
        order->next = nullptr;
        tail = order;
    }
    
    // Update level statistics
    total_volume_ += order->get_volume();
    order_count_++;
}

void PriceLevel::remove_order(Order* order) {
    // Verify the order belongs to this level
    if (order->level != this) {
        return;
    }
    
    // Update pointers to remove from linked list
    if (order->prev) {
        order->prev->next = order->next;
    } else {
        // This was the head
        head = order->next;
    }
    
    if (order->next) {
        order->next->prev = order->prev;
    } else {
        // This was the tail
        tail = order->prev;
    }
    
    // Update level statistics
    total_volume_ -= order->get_volume();
    order_count_--;
    
    // Clear the order's pointers
    order->prev = nullptr;
    order->next = nullptr;
    order->level = nullptr;
}

void PriceLevel::update_volume(Order* order, int old_volume) {
    // Verify order belongs to this level
    if (order->level != this) {
        return;
    }
    
    // Adjust total volume 
    total_volume_ = total_volume_ - old_volume + order->get_volume();
}

PriceLevel* PriceLevelList::find_level(const Price& price) {
    auto it = price_map_.find(price);
    if (it != price_map_.end()) {
        return it->second;
    }
    return nullptr;
}

PriceLevel* PriceLevelList::create_level(const Price& price) {
    // Check if level already exists
    PriceLevel* existing = find_level(price);
    if (existing) {
        return existing;
    }
    
    // Create new price level
    PriceLevel* new_level = pool_.allocate();
	new (new_level) PriceLevel(price);
    
    // Insert into the sorted linked list
    if (!head_) {
        // First level in the list
        head_ = tail_ = new_level;
        new_level->next_price = nullptr;
        new_level->prev_price = nullptr;
    } else {
        // Find the correct position to insert
        PriceLevel* current = head_;
        PriceLevel* prev = nullptr;
		(void) prev;
        
        bool inserted = false;
        
        if (is_bid_side_) {
            // For bid side, we want descending order (highest price first)
            while (current && !inserted) {
                if (price > current->get_price()) {
                    // Insert before current
                    new_level->next_price = current;
                    new_level->prev_price = current->prev_price;
                    
                    if (current->prev_price) {
                        current->prev_price->next_price = new_level;
                    } else {
                        head_ = new_level; // New head
                    }
                    
                    current->prev_price = new_level;
                    inserted = true;
                } else {
                    prev = current;
                    current = current->next_price;
                }
            }
            
            if (!inserted) {
                // Insert at the end
                tail_->next_price = new_level;
                new_level->prev_price = tail_;
                new_level->next_price = nullptr;
                tail_ = new_level;
            }
        } else {
            // For ask side, we want ascending order (lowest price first)
            while (current && !inserted) {
                if (price < current->get_price()) {
                    // Insert before current
                    new_level->next_price = current;
                    new_level->prev_price = current->prev_price;
                    
                    if (current->prev_price) {
                        current->prev_price->next_price = new_level;
                    } else {
                        head_ = new_level; // New head
                    }
                    
                    current->prev_price = new_level;
                    inserted = true;
                } else {
                    prev = current;
                    current = current->next_price;
                }
            }
            
            if (!inserted) {
                // Insert at the end
                tail_->next_price = new_level;
                new_level->prev_price = tail_;
                new_level->next_price = nullptr;
                tail_ = new_level;
            }
        }
    }
    
    // Add to price map for fast lookups
    price_map_[price] = new_level;
    
    return new_level;
}

void PriceLevelList::remove_level(PriceLevel* level) {
    // Verify level exists in our map
    auto it = price_map_.find(level->get_price());
    if (it == price_map_.end() || it->second != level) {
        return; // Not in our list
    }
    
    // Remove from linked list
    if (level->prev_price) {
        level->prev_price->next_price = level->next_price;
    } else {
        head_ = level->next_price; // Removing head
    }
    
    if (level->next_price) {
        level->next_price->prev_price = level->prev_price;
    } else {
        tail_ = level->prev_price; // Removing tail
    }
    
    // Remove from price map
    price_map_.erase(level->get_price());
}

PriceLevel* PriceLevelList::get_best_level() const {
    return head_; // Best is always at the head (highest bid or lowest ask)
}

bool PriceLevelList::empty() const {
    return head_ == nullptr;
}

PriceLevel* PriceLevelList::begin() const {
    return head_;
}

PriceLevel* PriceLevelList::next(PriceLevel* current) const {
    return current ? current->next_price : nullptr;
}

}
