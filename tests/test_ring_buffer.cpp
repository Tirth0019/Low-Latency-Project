#include "core/ring_buffer.hpp"
#include <iostream>
#include <thread>
#include <vector>
#include <cassert>

using namespace core;

void test_empty_buffer() {
    std::cout << "Running test_empty_buffer...\n";
    RingBuffer<int, 4> rb;
    int val = 0;
    
    // Pop on empty should return false immediately
    bool success = rb.pop(val);
    assert(!success && "pop() on empty buffer should return false");
    assert(rb.empty());
    assert(rb.size() == 0);
    
    std::cout << "test_empty_buffer passed.\n";
}

void test_full_buffer() {
    std::cout << "Running test_full_buffer...\n";
    RingBuffer<int, 4> rb;
    
    // Push exactly Capacity elements
    assert(rb.push(1));
    assert(rb.push(2));
    assert(rb.push(3));
    assert(rb.push(4));
    
    assert(!rb.empty());
    assert(rb.size() == 4);
    
    // Next push should fail (buffer full)
    bool success = rb.push(5);
    assert(!success && "push() on full buffer should return false");
    
    // Pop one and push again should succeed
    int val = 0;
    assert(rb.pop(val));
    assert(val == 1);
    assert(rb.push(5));
    
    std::cout << "test_full_buffer passed.\n";
}

void test_round_trip() {
    std::cout << "Running test_round_trip (concurrent)...\n";
    
    constexpr std::size_t kCapacity = 1024;
    constexpr std::size_t kItems = 100000;
    RingBuffer<int, kCapacity> rb;
    
    std::thread producer([&rb]() {
        for (std::size_t i = 0; i < kItems; ++i) {
            // Spin-wait until push succeeds
            while (!rb.push(static_cast<int>(i))) {
                // optional: std::this_thread::yield(); 
            }
        }
    });
    
    std::thread consumer([&rb]() {
        for (std::size_t i = 0; i < kItems; ++i) {
            int val = -1;
            // Spin-wait until pop succeeds
            while (!rb.pop(val)) {
                // optional: std::this_thread::yield();
            }
            assert(val == static_cast<int>(i) && "Data corrupted or lost in ring buffer!");
        }
    });
    
    producer.join();
    consumer.join();
    
    std::cout << "test_round_trip passed.\n";
}

int main() {
    std::cout << "Starting RingBuffer tests...\n";
    
    test_empty_buffer();
    test_full_buffer();
    test_round_trip();
    
    std::cout << "All RingBuffer tests passed!\n";
    return 0;
}
