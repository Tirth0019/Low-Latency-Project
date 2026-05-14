#pragma once

#include "order/price_level.hpp"
#include <map>
#include <unordered_map>
#include <memory>
#include "core/memory.hpp"

namespace engine {
    class MatchingEngine; // Forward declaration
}

namespace order {

using namespace core;

class OrderBook {
public:
    OrderBook();

    // O(1) best bid/ask
    bool has_bids() const { return best_bid_ != bids_.end(); }
    bool has_asks() const { return best_ask_ != asks_.end(); }

    const PriceLevel& get_best_bid() const { return best_bid_->second; }
    const PriceLevel& get_best_ask() const { return best_ask_->second; }

    // Order operations
    Order* add(OrderId id, Side side, Price price, Quantity qty);
    bool cancel(OrderId id);
    bool modify(OrderId id, Quantity new_qty); // TODO: implement in-place qty reduction
    
    // Test utility
    std::size_t size() const { return order_map_.size(); }

private:
    friend class engine::MatchingEngine;

    // bids: highest price first
    std::map<Price, PriceLevel, std::greater<Price>> bids_;
    
    // asks: lowest price first
    std::map<Price, PriceLevel> asks_;

    // O(1) best bid/ask — cached iterators, invalidated on level removal
    decltype(bids_)::iterator best_bid_;
    decltype(asks_)::iterator best_ask_;

    // O(1) order lookup for cancel/modify
    std::unordered_map<OrderId, Order*> order_map_;

    // Object pool for fast allocation
    std::unique_ptr<core::memory::ObjectPool<Order, 65536>> pool_;

    // Internal helpers
    void refresh_best_bid_ask(Side side, Price price);
    void refresh_best_bid_ask();
    void add_resting(Order* order);
};

} // namespace order
