#ifndef LOWLATENCY_H
#define LOWLATENCY_H

#include <cstdint>
#include <chrono>

// Low latency utility functions

/**
 * Perform a low latency operation
 * This is a placeholder for your actual low latency implementation
 */
void performLowLatencyOperation();

/**
 * Get current timestamp in nanoseconds
 * @return Current timestamp as nanoseconds since epoch
 */
inline int64_t getTimestampNs() {
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
}

#endif // LOWLATENCY_H
