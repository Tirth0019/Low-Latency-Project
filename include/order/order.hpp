#pragma once

#include <cstdint>
#include "core/types.hpp"

namespace order {

using namespace core;

// Valid transitions:
// New -> PartFill -> Filled
// New -> Cancelled
// PartFill -> Cancelled
// (no other transitions are legal)
enum class OrderState : uint8_t {
    New, 
    PartFill, 
    Filled, 
    Cancelled
};

// POD-compatible for pool allocation except for intrusive list pointers 
// — zero-initialized by ObjectPool.
struct Order {
    OrderId   id;            // 8 bytes
    Price     price;         // 8 bytes
    Quantity  qty;           // 8 bytes
    Quantity  remaining_qty; // 8 bytes  <- first 32 bytes = 1 cache line
    Side      side;          // 1 byte
    OrderType type;          // 1 byte
    OrderState state;        // 1 byte
    uint8_t   _pad[5];       // explicit padding to 8-byte align timestamp
    uint64_t  timestamp_ns;  // 8 bytes (from MonotonicClock::now_ns())
    
    // intrusive list pointers for PriceLevel
    Order*    prev{nullptr};
    Order*    next{nullptr};
};

static_assert(sizeof(Order) == 64, "Order struct size changed — check alignment");

} // namespace order
