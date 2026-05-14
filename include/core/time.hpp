#pragma once

#include <chrono>
#include <cstdint>
#include "core/types.hpp"
#include <thread>

// Intrinsic headers for __rdtsc
#if defined(_MSC_VER)
    #include <intrin.h>
#elif defined(__GNUC__) || defined(__clang__)
    #include <x86intrin.h>
#endif

namespace core {

namespace time {

// Read Time Stamp Counter (TSC) for ultra-low-overhead timing
inline uint64_t rdtsc() {
#if defined(_MSC_VER)
    return __rdtsc();
#elif defined(__GNUC__) || defined(__clang__)
    return __rdtsc();
#else
    // Fallback if not x86/MSVC/GCC/Clang
    return std::chrono::high_resolution_clock::now().time_since_epoch().count();
#endif
}

class MonotonicClock {
public:
    // Returns current time in nanoseconds using a steady clock.
    // Maps to QueryPerformanceCounter on Windows and clock_gettime(CLOCK_MONOTONIC) on POSIX.
    static Timestamp now_ns() {
        const auto now = std::chrono::steady_clock::now();
        return static_cast<Timestamp>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count()
        );
    }
};

// Note on Calibration:
// For accurate latency tracking across different CPU power states or on cloud VMs, 
// a one-time calibration step should be performed at application startup.
// This calibration maps the TSC ticks (`rdtsc()`) to actual nanoseconds by taking
// a baseline measurement using `MonotonicClock::now_ns()` over a known interval.
// Example:
// uint64_t tsc_start = rdtsc();
// Timestamp ns_start = MonotonicClock::now_ns();
// // ... wait ...
// uint64_t tsc_end = rdtsc();
// Timestamp ns_end = MonotonicClock::now_ns();
// double tsc_to_ns_ratio = (double)(ns_end - ns_start) / (tsc_end - tsc_start);

inline Timestamp duration_ns(Timestamp start_ns, Timestamp end_ns) {
    return end_ns - start_ns;
}

// TSC calibration — required before p99 means anything
inline double calibrate_tsc_ns() {
    auto t0 = MonotonicClock::now_ns();
    uint64_t r0 = rdtsc();
    
    // Use standard sleep to avoid dependency on thread header if possible, 
    // but sleep_for is the most reliable cross-platform.
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    uint64_t r1 = rdtsc();
    auto t1 = MonotonicClock::now_ns();
    return (double)(t1 - t0) / (double)(r1 - r0); // ns per tick
}

} // namespace time
} // namespace core