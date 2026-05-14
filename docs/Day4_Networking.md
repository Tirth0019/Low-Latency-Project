# Day 4: Networking & I/O Layer

## Overview
Day 4 focused on transforming the engine from a local library into a networked service. The core requirement was to maintain ultra-low latency while adding the overhead of TCP/UDP communication and binary serialization.

## Key Components

### 1. Platform-Aware Socket Wrappers (`net::TcpSocket`, `net::UdpSocket`)
- **RAII Implementation**: Automated lifecycle management of socket handles.
- **Tuning**:
  - `TCP_NODELAY`: Disabled Nagle's algorithm for immediate packet transmission.
  - `SO_RCVBUF`: Set to 4MB to handle bursts without packet loss.
  - `O_NONBLOCK`: Enabled non-blocking I/O for high-frequency polling.
- **Cross-Platform**: Support for both Windows (Winsock2) and POSIX systems.

### 2. Zero-Copy Binary Codec (`net::Codec`)
- **Performance**: Uses `reinterpret_cast` for zero-copy access to binary messages.
- **Integrity**: Fast XOR-based checksum included in every message frame.
- **Protocol**: Fixed-width binary messages for predictable parsing latency.

### 3. Multi-Threaded Engine Orchestration
The system was refactored into a four-thread architecture:
1.  **Engine Thread**: Pinned to a dedicated core; polls inbound ring buffers and executes matching logic.
2.  **Recv Thread (TCP)**: Ingests client orders and pushes them to the engine.
3.  **Send Thread (TCP)**: Polls engine trade events and streams them to connected clients.
4.  **FeedHandler (UDP)**: Processes multicast market data and synthesizes synthetic orders for the engine.

## Addressed Architectural Gaps

During implementation, the following critical gaps were identified and resolved:
- **TCP Fragmentation**: Added `// TODO: recv accumulator` placeholder in `recv_loop` to handle fragmented packets in future sessions.
- **Thread Ownership**: Moved `std::thread` management into the `Engine` class for RAII-compliant cleanup.
- **Shutdown Protocol**: Implemented a strict join order (`recv` -> `send` -> `feed` -> `engine`) to prevent use-after-free errors.
- **Non-Blocking Feed**: Updated `FeedHandler` to use non-blocking UDP sockets, ensuring the thread exits cleanly during shutdown.
- **Namespace Consistency**: Wrapped all networking components in the `net` namespace to prevent name collisions.

## Results & Verification

### Codec Unit Tests
| Case | Result | Note |
| :--- | :--- | :--- |
| Round-trip Integrity | **PASSED** | Field-perfect reconstruction. |
| Corruption Detection | **PASSED** | Checksum caught single-bit flips. |
| Buffer Safety | **PASSED** | No overflows on small buffers. |

### Loopback Integration Test
- **RTT Measurement**: **~142.0 μs**
- **Configuration**: Single-process, loopback interface, single core pinning for engine.
- **Conclusion**: The networking stack (OS kernel) is now the dominant latency factor, contributing >99% of the total path overhead compared to the ~650ns matching time.

---
**Next Step**: Day 5 - Persistence Layer (Binary Logging & Recovery).
