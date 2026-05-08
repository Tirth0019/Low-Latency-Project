#pragma once

#include <atomic>
#include <cstddef>
#include <new>

namespace core {

// Single-Producer Single-Consumer (SPSC) Lock-Free Ring Buffer
// Designed as an inter-thread IPC primitive.
template <typename T, std::size_t Capacity>
class RingBuffer {
    // Enforce power-of-2 capacity at compile time for fast modulo masking
    static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be a power of 2");
    static_assert(Capacity > 0, "Capacity must be greater than 0");

public:
    RingBuffer() = default;

    // Delete copy and move semantics
    RingBuffer(const RingBuffer&) = delete;
    RingBuffer& operator=(const RingBuffer&) = delete;
    RingBuffer(RingBuffer&&) = delete;
    RingBuffer& operator=(RingBuffer&&) = delete;

    // Push an element into the buffer (Producer thread only)
    // Returns true on success, false if the buffer is full.
    bool push(const T& item) {
        const std::size_t current_tail = tail_.val.load(std::memory_order_relaxed);
        const std::size_t next_tail = current_tail + 1;

        // Check if buffer is full. 
        // We load head with acquire to synchronize with the consumer's store to head.
        if (next_tail - head_.val.load(std::memory_order_acquire) > Capacity) {
            return false; // Buffer full
        }

        buffer_[current_tail & kMask] = item;
        
        // Release semantics ensure the item write is visible before the tail update is visible
        tail_.val.store(next_tail, std::memory_order_release);
        return true;
    }

    // Pop an element from the buffer (Consumer thread only)
    // Returns true on success, false if the buffer is empty.
    bool pop(T& item) {
        const std::size_t current_head = head_.val.load(std::memory_order_relaxed);

        // Check if buffer is empty.
        // We load tail with acquire to synchronize with the producer's store to tail.
        if (current_head == tail_.val.load(std::memory_order_acquire)) {
            return false; // Buffer empty
        }

        item = buffer_[current_head & kMask];

        // Release semantics ensure the item read is complete before the head update is visible
        head_.val.store(current_head + 1, std::memory_order_release);
        return true;
    }

    // Returns current number of elements
    std::size_t size() const {
        std::size_t h = head_.val.load(std::memory_order_acquire);
        std::size_t t = tail_.val.load(std::memory_order_acquire);
        return t - h;
    }

    bool empty() const {
        return size() == 0;
    }

private:
    static constexpr std::size_t kMask = Capacity - 1;

    // Aligned atomic to prevent false sharing between producer (updates tail)
    // and consumer (updates head). They must sit on separate cache lines.
    struct alignas(64) AlignedAtomic {
        std::atomic<std::size_t> val{0};
    };

    // Note: order of members matters for caching.
    // Ensure buffer array doesn't share a cache line with atomics if possible,
    // though the alignment of AlignedAtomic guarantees separation between head/tail.
    
    AlignedAtomic head_; // Written by consumer, read by producer
    AlignedAtomic tail_; // Written by producer, read by consumer

    T buffer_[Capacity];
};

} // namespace core
