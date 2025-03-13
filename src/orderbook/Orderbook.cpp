#include "../../include/orderbook/Orderbook.h"
#include "../../include/orderbook/OrderbookImpl.h"

namespace trading {

// Constructor
Orderbook::Orderbook() : 
	impl_(std::make_unique<OrderbookImpl>()) 
{}

// Destructor
Orderbook::~Orderbook() = default;

// Move constructor
Orderbook::Orderbook(Orderbook&& other) noexcept :
	impl_(std::move(other.impl_))
{}

// Move assignment
Orderbook& Orderbook::operator=(Orderbook&& other) noexcept {
    if (this != &other) {
        impl_ = std::move(other.impl_);
    }
    return *this;
}

// Core functionality
OrderResult Orderbook::place_order(const Order& order, std::vector<TradeInfo>& trades_executed) {
    return impl_->place_order(order, trades_executed);
}

OrderResult Orderbook::cancel_order(int order_id) {
    return impl_->cancel_order(order_id);
}

OrderResult Orderbook::modify_order(int order_id, const Price& new_price, int new_volume) {
    return impl_->modify_order(order_id, new_price, new_volume);
}

std::vector<BookLevel> Orderbook::get_bid_levels(int depth) const {
    return impl_->get_bid_levels(depth);
}

std::vector<BookLevel> Orderbook::get_ask_levels(int depth) const {
    return impl_->get_ask_levels(depth);
}

Price Orderbook::get_mid_price() const {
    return impl_->get_mid_price();
}

Price Orderbook::get_best_bid() const {
    return impl_->get_best_bid();
}

Price Orderbook::get_best_ask() const {
    return impl_->get_best_ask();
}

int Orderbook::get_volume_at_price(const Price& price, Side side) const {
    return impl_->get_volume_at_price(price, side);
}

// Metrics
size_t Orderbook::order_count() const {
	return impl_->order_count();
}

size_t Orderbook::price_level_count() const {
	return impl_->price_level_count();
}

// Debug
void Orderbook::print_book() const {
    return impl_->print_book();
}

}
