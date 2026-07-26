#pragma once

#include <cstddef>
#include <cstdint>
#include <cassert>
#include <vector>

#include "../common/FixedPoint.h"

namespace trading {

using ClientId = std::uint32_t;
using OrderId = std::uint64_t;
using Quantity = std::int32_t;

enum class Side : std::uint8_t {
    BUY,
    SELL,
};

enum class OrderResult : std::uint8_t {
    SUCCESS,
    PARTIAL_FILL,
    COMPLETE_FILL,
    INVALID_ORDER,
    ORDER_NOT_FOUND,
    DUPLICATE_ORDER_ID,
    BOOK_FULL,
    PRICE_LEVEL_LIMIT,
    EXECUTION_BUFFER_TOO_SMALL,
};

struct Execution {
    OrderId taker_order_id = 0;
    OrderId maker_order_id = 0;
    ClientId taker_client_id = 0;
    ClientId maker_client_id = 0;
    Price price{};
    Quantity quantity = 0;
    Side taker_side = Side::BUY;
};

class ExecutionBuffer {
public:
    explicit ExecutionBuffer(std::size_t capacity) : executions_(capacity) {}

    void clear() { size_ = 0; }
    std::size_t size() const { return size_; }
    std::size_t capacity() const { return executions_.size(); }
    const Execution* data() const { return executions_.data(); }
    const Execution& operator[](std::size_t index) const { return executions_[index]; }

    void push_unchecked(const Execution& execution) {
        assert(size_ < executions_.size());
        executions_[size_++] = execution;
    }

private:
    std::vector<Execution> executions_;
    std::size_t size_ = 0;
};

struct BookLevel {
    Price price{};
    Quantity total_volume = 0;
    std::size_t order_count = 0;
};

struct OrderbookConfig {
    std::size_t max_orders = 4096;
    std::size_t max_price_levels = 1024;
};

} // namespace trading
