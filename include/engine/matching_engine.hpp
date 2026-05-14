#pragma once

#include <vector>
#include "order/order_book.hpp"

namespace engine {

using namespace core;
using namespace order;

class MatchingEngine {
public:
    MatchingEngine() = default;

    // Process a new incoming limit order.
    // Returns a list of TradeEvents if any matches occurred.
    std::vector<TradeEvent> process_new_order(OrderId id, Side side, Price price, Quantity qty);

    // Cancel an existing order.
    bool process_cancel_order(OrderId id);

    // Get reference to the internal order book (for testing/inspection)
    const OrderBook& get_book() const { return book_; }

private:
    OrderBook book_;

    // Core matching logic
    std::vector<TradeEvent> match(Order* aggressive);
};

} // namespace engine
