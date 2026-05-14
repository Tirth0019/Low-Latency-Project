# Day 2: Order Model and Matching Engine Foundation

## Overview
This document summarizes the architecture, design choices, and implementation details for the core matching engine components created during Day 2 of the Ultra-Low-Latency Order Book Engine project.

## What Was Accomplished

### 1. Cache-Friendly Order Layout (`include/order/order.hpp`)
We designed the `Order` struct with a highly optimized memory layout prioritizing the CPU cache.
- The hottest matching fields (`id`, `price`, `qty`, `remaining_qty`) were packed perfectly into the first 32 bytes (a single cache line). 
- We verified cache line alignment by measuring the compiler output and locking the struct size with a `static_assert(sizeof(Order) == 64)`. The explicit 64-byte breakdown is:
  - **32 bytes** for the primary fields (4 × 8 bytes)
  - **8 bytes** for states and padding (`Side`, `OrderType`, `OrderState`, plus a 5-byte padding array)
  - **8 bytes** for `timestamp_ns`
  - **16 bytes** for the `prev` and `next` pointers.
- Integrated `enum class OrderState` with strict transition paths (New -> PartFill -> Filled, New -> Cancelled, PartFill -> Cancelled).

### 2. Zero-Allocation Intrusive List (`include/order/price_level.hpp`)
We built the `PriceLevel` logic around an intrusive doubly linked list design to eliminate latency spikes.
- Rather than relying on `std::list` (which triggers a heap allocation for every node wrapper), we embedded `prev` and `next` pointers directly into the `Order` struct.
- The `remove()` operation handles all 4 pointer cases perfectly, keeping operations strictly `O(1)` with exactly zero allocations on the hot path.
- Created the `TradeEvent` struct to formally represent matched trades.

### 3. O(1) Order Book Operations (`include/order/order_book.hpp`)
The central order book handles limits, cancels, and modifies.
- Built with two `std::map`s (`bids_` and `asks_`) and a fast `std::unordered_map` (`order_map_`) for `O(1)` cancel lookups.
- Implemented crucial `best_bid_` and `best_ask_` caching iterators. To eliminate the standard "Stale Iterators" bug, these caches are automatically updated via a private `refresh_best_bid_ask()` helper on every structural mutation.
- Injected a `std::hash` specialization for `core::StrongType` directly into `core/types.hpp`. This allows our strong types (like `OrderId`) to be used safely and seamlessly as keys in the unordered map.

### 4. Asymmetric Matching Engine (`src/engine/matching_engine.cpp`)
The matching loop was constructed with strict crossing logic.
- Flawlessly iterates over the resting price levels, executing asymmetric cross evaluations depending on whether the incoming order is a Buy or Sell.
- Correctly manages partial fills: splitting quantities, updating `OrderState` to `PartFill` or `Filled`, and securely erasing empty `PriceLevel`s from the map immediately to prevent referencing dangling pointers.
- Dynamically allocates from the Day 1 `ObjectPool` and re-inserts unfilled remainders as resting orders.

### 5. Verification & Testing (`tests/order_book_test.cpp`)
The system was verified against 8 strict test cases.
- Validated correct reporting of Best Bid/Ask during adds and cancels.
- Tested partial fills where aggressive quantity > passive quantity, and vice-versa.
- Addressed a potential stack-overflow trap: `ObjectPool` currently allocates statically. Allocating the `MatchingEngine` automatically places the 4.5MB pool on the thread stack, blowing past the default Windows 1MB limit. 
- **Fix:** We mitigated this in testing by dynamically allocating the engine (`std::make_unique<MatchingEngine>()`). 
- **Next Steps:** A Day 3 TODO was added to transition the `pool_` to be heap-allocated, as it will be necessary when embedding the engine inside the orchestration loop.

---
*All components obey the strict left-to-right dependency order, compile with zero warnings, and execute cleanly under MSVC.*
