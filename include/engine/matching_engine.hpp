#pragma once

#include <vector>
#include "order/order_book.hpp"

namespace engine {

using namespace core;
using namespace order;

class MatchingEngine {
public:
    explicit MatchingEngine(order::OrderBook& book) : book_(book) {}

    // Process a new incoming limit order.
    // Returns a list of TradeEvents if any matches occurred.
    std::vector<order::TradeEvent> process_new_order(core::OrderId id, core::Side side, core::Price price, core::Quantity qty);

    // Cancel an existing order.
    bool process_cancel_order(core::OrderId id);

    // Core matching logic
    std::vector<order::TradeEvent> match(order::Order* aggressive);

    // Get reference to the internal order book (for testing/inspection)
    const order::OrderBook& get_book() const { return book_; }
    order::OrderBook& get_book() { return book_; }

private:
    order::OrderBook& book_;
};

} // namespace engine
