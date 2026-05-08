# Day 1 Core Layer Implementation Summary

## Overview
This document summarizes the changes and additions made to the Core Layer foundation of the Ultra-Low-Latency Order Book Engine on Day 1. The focus was on setting up robust, low-latency, and thread-safe primitives.

## Implemented Components

### 1. Types (`include/core/types.hpp`)
- Implemented `StrongType` wrapper template.
- Defined `OrderId`, `Price`, and `Quantity` as strong types using tag dispatch to prevent accidental implicit conversions.
- Added `Side` and `OrderType` enumerations with a `uint8_t` underlying type for memory efficiency.

### 2. Time (`include/core/time.hpp`)
- Added inline `rdtsc()` wrapper utilizing compiler intrinsics (`__rdtsc()`) for ultra-low-overhead timestamps.
- Implemented `MonotonicClock::now_ns()` using `std::chrono::steady_clock` for cross-platform compatibility. 
  - *Note on naming:* The initial plan specified `MonotonicClock::now()`, but this was shipped as `now_ns()` to explicitly communicate the unit of return. Subsequent layers should call `now_ns()`.
- **Calibration Required:** TSC frequency on cloud VMs can differ from nominal CPU frequency — calibration at startup is required before TSC deltas can be converted to nanoseconds.

### 3. Memory (`include/core/memory.hpp`)
- Created `ObjectPool<T, Capacity>` template for fixed-size pool allocations to avoid heap allocations on the hot path.
- **Exhaustion Behavior:** If the pool reaches capacity, `allocate()` safely returns `nullptr`. The upper layers (e.g., the engine) are expected to handle this failure gracefully.
- Added a `used()` metric method to allow the engine to log and debug pool utilization.
- **Alignment:** Enforced explicit cache-line alignment using `alignas(64 > alignof(T) ? 64 : alignof(T))`. This ensures SIMD/cache-line friendly access and prevents false sharing, which is critical for the memory layouts of frequently accessed objects.

### 4. Ring Buffer (`include/core/ring_buffer.hpp`)
- Developed a lock-free Single-Producer Single-Consumer (SPSC) `RingBuffer<T, Capacity>`.
- **Compile-time Checks:** Added `static_assert` to ensure the capacity is strictly a power of 2 for fast modulo masking.
- **Cache Line Padding:** Applied `alignas(64)` to the `head_` and `tail_` atomic indices to prevent false sharing and destructive cache thrashing between the producer and consumer threads.
- Used `std::memory_order_acquire` and `std::memory_order_release` for safe inter-thread synchronization.

## Testing and Build Integration
- Added `tests/test_ring_buffer.cpp` which explicitly tests three critical edge cases:
  - **Round-trip Behavior**: Concurrent producer/consumer execution.
  - **Full Buffer Behavior**: Pushing to a full buffer safely returns `false`.
  - **Empty Buffer Behavior**: Popping an empty buffer safely returns `false`.
- Updated `CMakeLists.txt` to include `test_ring_buffer` as an executable target for automated building and testing.

## Next Steps
- Compile and run the core tests under `-Wall -Wextra` (or `/W4`) and ThreadSanitizer (`-fsanitize=thread`) in the appropriate MSVC Developer Command Prompt or Linux environment to ensure thread safety.
- **Day 2 will build the Order model and price-level order book on top of these primitives — `ObjectPool` will serve as the Order allocator.**
