#pragma once
#include <vector>
#include <string>
#include <ctime>
#include "../common/FixedPoint.h"
#include <unordered_map>

namespace trading {

// Order side
enum class Side {
    BUY, 
    SELL
};

// Result of order operations
enum class OrderResult {
    SUCCESS,
    PARTIAL_FILL,
    COMPLETE_FILL,
    REJECTED,
    INVALID_ORDER,
    ORDER_NOT_FOUND,
    DUPLICATE_ORDER_ID
};

// Trrade execution information
struct TradeInfo {
    int order_id;
    std::string client_name;
    Price price;
    int volume;
    bool is_buy;
    std::string counterparty;
};

// Market data price level
struct BookLevel {
    Price price;
    int total_volume;
    int order_count;
};

}
