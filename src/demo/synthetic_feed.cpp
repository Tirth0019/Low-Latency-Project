#include <iostream>
#include <random>
#include <thread>
#include <vector>
#include <chrono>
#include <algorithm>
#include <functional>

#include "engine/engine.hpp"
#include "core/time.hpp"

#ifdef _WIN32
#include <intrin.h>
#else
#include <x86intrin.h>
#endif

using namespace core;
using namespace order;
using namespace engine;

// TSC calibration — required before p99 means anything
double calibrate_tsc_ns() {
    auto t0 = core::time::MonotonicClock::now_ns();
#ifdef _WIN32
    uint64_t r0 = __rdtsc();
#else
    uint64_t r0 = __rdtsc();
#endif
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
#ifdef _WIN32
    uint64_t r1 = __rdtsc();
#else
    uint64_t r1 = __rdtsc();
#endif
    auto t1 = core::time::MonotonicClock::now_ns();
    return (double)(t1 - t0) / (double)(r1 - r0); // ns per tick
}

void synthetic_feed(RingBuffer<Order, 4096>& ring, int count) {
    std::mt19937 rng(42);
    std::uniform_int_distribution<> price_dist(99, 101); // tight spread
    std::uniform_int_distribution<> qty_dist(1, 100);
    std::uniform_int_distribution<> side_dist(0, 1);

    for (int i = 0; i < count; ++i) {
        Order o{};
        o.id    = OrderId{(uint64_t)i};
        o.price = Price{(int64_t)price_dist(rng)};
        o.qty   = Quantity{(uint64_t)qty_dist(rng)};
        o.remaining_qty = o.qty;
        o.side  = side_dist(rng) ? Side::Buy : Side::Sell;
        o.state = OrderState::New;
        o.timestamp_ns = core::time::MonotonicClock::now_ns();

        while (!ring.push(o)) { /* spin until space */ }
    }
}

void print_latency_report(std::vector<uint64_t>& samples, double ns_per_tick) {
    if (samples.empty()) {
        std::cout << "No samples collected.\n";
        return;
    }
    std::sort(samples.begin(), samples.end());
    size_t n = samples.size();
    double p50  = samples[n * 0.50] * ns_per_tick;
    double p99  = samples[n * 0.99] * ns_per_tick;
    double p999 = samples[n * 0.999] * ns_per_tick;

    printf("Processed %zu orders\n", n);
    printf("p50  latency: %.1f ns\n", p50);
    printf("p99  latency: %.1f ns\n", p99);
    printf("p999 latency: %.1f ns\n", p999);
}

int main() {
    std::cout << "Calibrating TSC... this will take 100ms.\n";
    double ns_per_tick = calibrate_tsc_ns();
    std::cout << "TSC calibration: " << ns_per_tick << " ns/tick\n";

    RiskLimits limits{
        Quantity{1000},          // max_order_qty
        1'000'000,               // max_notional
        10000,                   // max_long_position
        -10000                   // max_short_position
    };

    Engine engine(limits);
    engine.pin_to_core(0); // Pin to core 0 (stub on Windows)

    constexpr int kOrderCount = 100'000;

    // Start engine in background thread
    std::thread engine_thread([&engine]() {
        engine.run();
    });

    // To verify Risk rejects oversized orders correctly:
    // We can inject one oversized order first.
    std::cout << "Sending 1 oversized order to test risk limits...\n";
    Order big{};
    big.id = OrderId{999999};
    big.price = Price{100};
    big.qty = Quantity{5000}; // Exceeds max_order_qty (1000)
    big.remaining_qty = big.qty;
    big.side = Side::Buy;
    big.state = OrderState::New;
    big.timestamp_ns = core::time::MonotonicClock::now_ns();
    while (!engine.get_inbound_ring().push(big)) {}

    // Wait for the engine to process it
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    // Start synthetic feed thread
    std::cout << "Starting synthetic feed for " << kOrderCount << " orders...\n";
    std::thread feed_thread(synthetic_feed, std::ref(engine.get_inbound_ring()), kOrderCount);

    feed_thread.join();
    
    // Give engine time to process the last orders
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    engine.stop();
    engine_thread.join();

    print_latency_report(engine.get_latency_samples(), ns_per_tick);

    return 0;
}
