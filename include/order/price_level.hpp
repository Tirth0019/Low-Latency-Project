#pragma once

#include <cstdint>
#include "order/order.hpp"

namespace order {

using namespace core;

// TradeEvent: Represents a matched trade between two orders.
struct TradeEvent {
    OrderId  aggressive_id;
    OrderId  passive_id;
    Price    trade_price;
    Quantity trade_qty;
    uint64_t timestamp_ns;
};

// PriceLevel: An intrusive doubly linked list of orders at a specific price.
// We use an intrusive list instead of std::list<Order*> because std::list 
// allocates a node wrapper per element on the heap. An intrusive list stores 
// prev/next directly inside the Order struct, meaning push/pop/remove are 
// pure pointer manipulations with zero allocation overhead.
struct PriceLevel {
    Price    price;
    uint64_t total_qty{0};  // sum of remaining_qty at this level
    uint32_t order_count{0};
    Order*   head{nullptr}; // front of FIFO queue (oldest = first to match)
    Order*   tail{nullptr}; // back (newest resting order)

    // Add order to the back of the queue (new resting order)
    void push_back(Order* o) {
        o->prev = tail;
        o->next = nullptr;
        
        if (tail) {
            tail->next = o;
        } else {
            head = o;
        }
        tail = o;
        
        total_qty += o->remaining_qty;
        ++order_count;
    }

    // Remove order from the queue in O(1) time
    void remove(Order* o) {
        if (o->prev) {
            o->prev->next = o->next;
        } else {
            head = o->next;      // o was head
        }
        
        if (o->next) {
            o->next->prev = o->prev;
        } else {
            tail = o->prev;      // o was tail
        }
        
        o->prev = o->next = nullptr;
        total_qty -= o->remaining_qty;
        --order_count;
    }

    bool empty() const { 
        return head == nullptr; 
    }
};

} // namespace order
