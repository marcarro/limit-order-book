#include <stdlib.h>

#include "Orderbook.h"
#include "OrderbookImpl.h"

// Constructor/destructor
Orderbook::Orderbook() : impl_(std::make_unique<OrderbookImpl>()) {}
Orderbook::~Orderbook() = default;

// Forward all public calls to implementation
Orderbook::Result Orderbook::place_order(const Order& order, std::vector<TradeInfo>& trades_executed) {
    return impl_->place_order(order, trades_executed);
}

Orderbook::Result Orderbook::cancel_order(int order_id) {
    return impl_->cancel_order(order_id);
}

Orderbook::Result Orderbook::modify_order(int order_id, double new_price, int new_volume) {
    return impl_->modify_order(order_id, new_price, new_volume);
}

std::vector<Orderbook::BookLevel> Orderbook::get_bid_levels(int depth) const {
    return impl_->get_bid_levels(depth);
}

std::vector<Orderbook::BookLevel> Orderbook::get_ask_levels(int depth) const {
    return impl_->get_ask_levels(depth);
}

double Orderbook::get_mid_price() const {
    return impl_->get_mid_price();
}

double Orderbook::get_best_bid() const {
    return impl_->get_best_bid();
}

double Orderbook::get_best_ask() const {
    return impl_->get_best_ask();
}

int Orderbook::get_volume_at_price(double price, buy_or_sell side) const {
    return impl_->get_volume_at_price(price, side);
}

void Orderbook::print_book() const {
    return impl_->print_book();
}
