#pragma once

#include <atomic>
#include <cstdint>
#include <cstdio>
#include <algorithm>

#ifdef _MSC_VER
#include <intrin.h>
#endif

namespace metrics {

class LatencyTracker {
    static constexpr int NUM_BUCKETS = 64;

    // Cache-line alignment to prevent false sharing between buckets
    struct alignas(64) Bucket {
        std::atomic<uint64_t> count{0};
    };

    Bucket buckets_[NUM_BUCKETS];
    double ns_per_tick_{1.0};

public:
    explicit LatencyTracker(double ns_per_tick) : ns_per_tick_(ns_per_tick) {
        reset();
    }

    // Called on engine hot path — must be lock-free and extremely fast
    void record(uint64_t tsc_delta) noexcept {
        int bucket = 0;
#ifdef _MSC_VER
        unsigned long idx;
        if (_BitScanReverse64(&idx, tsc_delta | 1)) {
            bucket = (int)idx;
        }
#else
        bucket = 63 - __builtin_clzll(tsc_delta | 1);
#endif
        // Relaxed ordering is sufficient for recording; we just need eventual consistency
        buckets_[bucket].count.fetch_add(1, std::memory_order_relaxed);
    }

    void reset() {
        for (int i = 0; i < NUM_BUCKETS; ++i) {
            buckets_[i].count.store(0, std::memory_order_relaxed);
        }
    }

    // Called from reporting thread
    void report() const {
        uint64_t total = 0;
        for (int i = 0; i < NUM_BUCKETS; ++i) {
            total += buckets_[i].count.load(std::memory_order_relaxed);
        }

        if (total == 0) {
            printf("LatencyTracker: No samples collected.\n");
            return;
        }

        const uint64_t targets[] = {total / 2, total * 95 / 100, total * 99 / 100, total * 999 / 1000};
        const char* labels[] = {"p50", "p95", "p99", "p999"};
        
        uint64_t cumulative = 0;
        int t = 0;
        
        printf("--- Latency Report (%llu samples) ---\n", (unsigned long long)total);
        for (int i = 0; i < NUM_BUCKETS && t < 4; ++i) {
            cumulative += buckets_[i].count.load(std::memory_order_relaxed);
            while (t < 4 && cumulative >= targets[t]) {
                // Each bucket i represents range [2^i, 2^(i+1)) ticks
                // We report the lower bound * ns_per_tick
                double ns = (1ULL << i) * ns_per_tick_;
                printf("%-6s %.1f ns\n", labels[t], ns);
                ++t;
            }
        }
        printf("----------------------------------\n");
    }

    uint64_t get_percentile(double pct) const {
        uint64_t total = 0;
        for (int i = 0; i < NUM_BUCKETS; ++i)
            total += buckets_[i].count.load(std::memory_order_relaxed);
        if (total == 0) return 0;

        uint64_t target = (uint64_t)(total * pct);
        uint64_t cumulative = 0;
        for (int i = 0; i < NUM_BUCKETS; ++i) {
            cumulative += buckets_[i].count.load(std::memory_order_relaxed);
            if (cumulative >= target)
                return (uint64_t)((1ULL << i) * ns_per_tick_);
        }
        return 0;
    }

    uint64_t get_p50()  const { return get_percentile(0.50); }
    uint64_t get_p99()  const { return get_percentile(0.99); }
    uint64_t get_p999() const { return get_percentile(0.999); }
};

} // namespace metrics
