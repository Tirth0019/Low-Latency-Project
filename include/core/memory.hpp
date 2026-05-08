#pragma once

#include <cstddef>
#include <vector>
#include <stdexcept>
#include <new>

namespace core {
namespace memory {

// ObjectPool: A fixed-size, pre-allocated pool allocator.
// Ensures no heap allocations happen on the hot path.
template <typename T, std::size_t Capacity>
class ObjectPool {
public:
    ObjectPool() : free_index_(0) {
        // Pre-allocate pointers to all elements
        free_list_.reserve(Capacity);
        for (std::size_t i = 0; i < Capacity; ++i) {
            free_list_.push_back(&pool_[i]);
        }
    }

    // Delete copy/move constructors to prevent accidental copies of the pool
    ObjectPool(const ObjectPool&) = delete;
    ObjectPool& operator=(const ObjectPool&) = delete;
    ObjectPool(ObjectPool&&) = delete;
    ObjectPool& operator=(ObjectPool&&) = delete;

    // Allocate an object from the pool
    template <typename... Args>
    T* allocate(Args&&... args) {
        if (free_index_ >= Capacity) {
            return nullptr; // Pool exhausted
        }
        
        T* ptr = free_list_[free_index_++];
        // In-place construct
        return new (ptr) T(std::forward<Args>(args)...);
    }

    // Deallocate an object back to the pool
    void deallocate(T* ptr) {
        if (!ptr) return;
        
        // Explicitly call the destructor
        ptr->~T();
        
        if (free_index_ > 0) {
            free_list_[--free_index_] = ptr;
        }
    }

    constexpr std::size_t capacity() const {
        return Capacity;
    }

    std::size_t available() const {
        return Capacity - free_index_;
    }

    std::size_t used() const {
        return free_index_;
    }

private:
    // Ensure memory is aligned to at least the size of a cache line (64 bytes)
    // or the alignment requirement of T, whichever is strictly greater,
    // to prevent false sharing and ensure SIMD/cache-friendly access.
    // However, alignas(64) on the array element itself enforces padding.
    struct alignas(64 > alignof(T) ? 64 : alignof(T)) AlignedStorage {
        alignas(T) std::byte data[sizeof(T)];

        // Helper to cast to T*
        operator T*() { return reinterpret_cast<T*>(data); }
    };

    AlignedStorage pool_[Capacity];
    
    // We use a vector for the free list for simplicity.
    // In a pure lock-free environment, this could be a lock-free stack, 
    // but typically a pool is per-thread or protected.
    std::vector<T*> free_list_;
    std::size_t free_index_;
};

} // namespace memory
} // namespace core