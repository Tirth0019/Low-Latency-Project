# System Design & Architecture Decisions

This document outlines the core technical decisions made to achieve sub-microsecond matching latency and deterministic recovery.

## Why SPSC Ring Buffers over Mutex Queues?
Traditional thread-safe queues use `std::mutex` to protect shared state, which introduces significant overhead. Mutex acquisition on the hot path typically costs 20–100ns per lock/unlock cycle, and if contention occurs, the OS may suspend the thread, causing latency spikes in the microsecond range. This engine uses Single-Producer-Single-Consumer (SPSC) ring buffers with acquire-release memory ordering, costing only ~2–5ns per operation. By design, there is exactly one producer and one consumer for each buffer, eliminating contention and the need for expensive kernel-level locking primitives.

## Why Pool Allocator over `new`/`delete`?
Standard heap allocation via `operator new` or `malloc` is non-deterministic and unsuitable for low-latency hot paths. The OS allocator holds global locks to manage fragmented heaps and may trigger expensive page faults or memory-reclamation sweeps, resulting in latencies ranging from 200ns to 5μs. Our system uses a pre-allocated `ObjectPool` created at startup, ensuring that all `Order` objects are contiguous in memory. Allocation from the pool is a simple pointer-bump operation (~2ns), guaranteeing constant-time performance and superior cache locality.

## Why Intrusive Linked Lists for Price Levels?
Standard containers like `std::list<Order*>` allocate a separate "node" wrapper on the heap for every element, which leads to pointer chasing and cache misses. In our intrusive design, the `prev` and `next` pointers are members of the `Order` struct itself, which is already pool-allocated. This allows the `MatchingEngine` to traverse or remove orders from a `PriceLevel` with zero additional allocations or deallocations. Canceling an order becomes a simple $O(1)$ pointer manipulation, which is significantly faster and more cache-efficient than managing external list nodes.
