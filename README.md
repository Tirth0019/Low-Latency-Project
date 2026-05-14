# Ultra-Low-Latency Order Book Engine

A high-performance C++20 matching engine optimized for sub-microsecond latency, featuring lock-free orchestration, binary journaling, and a decoupled networking layer.

## Performance Benchmarks

*Recorded on Windows via MSVC (`__rdtsc`) with CPU Turbo disabled.*

| Operation | Latency (ns) | Throughput |
| :--- | :--- | :--- |
| **Add Order (Limit)** | ~81.5 ns | ~12.2 M/s |
| **Cancel Order** | ~118.7 ns | ~8.4 M/s |
| **Matching (Core)** | ~110.2 ns | ~9.0 M/s |
| **Journaling (WAL)** | +75.3 ns | - |
| **Total Engine Path** | **~185.5 ns** | **~5.4 M/s** |
| **Loopback RTT** | ~142,000 ns | - |

> [!NOTE]
> The matching engine scales **O(1)** with book depth. Benchmarks confirmed nearly identical latencies at 10, 100, and 1,000 price levels due to intrusive list pointers and iterator caching.

## Core Features

- **Lock-Free Orchestration**: Uses SPSC ring buffers for inter-thread communication, ensuring the engine thread never stalls on I/O.
- **Deterministic Matching**: Sub-200ns matching path with branchless risk checks and intrusive order book.
- **Ultra-Fast Persistence**: Append-only binary Journal (WAL) with 512MB pre-allocation to eliminate filesystem metadata spikes.
- **Metrics & Telemetry**: Lock-free HDR Histogram for real-time p50/p99 reporting with zero jitter impact.
- **Decoupled Networking**: Dedicated Recv/Send threads handling TCP order entry and UDP multicast market data feeds.
- **Memory Efficiency**: Custom `ObjectPool` for allocation-free hot path and zero-copy binary codecs.

## Architecture

The engine employs a **multi-threaded, lock-free architecture** designed to keep the matching hot-path pinned and undisturbed. I/O operations (TCP/UDP) are offloaded to dedicated threads that communicate with the core engine via Single-Producer-Single-Consumer (SPSC) ring buffers. This design ensures that networking jitter and disk I/O latency never stall the matching loop. The matching engine itself is strictly deterministic, allowing for perfect state reconstruction during Journal replay.

## Prerequisites

### Required
- **C++20 compatible compiler**:
  - **Windows**: Visual Studio 2019+ (MSVC) or MinGW-w64 (GCC 10+)
  - **Linux**: GCC 10+ or Clang 12+
  - **macOS**: Xcode 12+ (Clang)

### Optional (for CMake builds)
- **CMake 3.15 or higher**
- **Visual Studio** (for Windows) - includes CMake support

## Project Structure

```
Ultra-Low-Latency-Order-Book-Engine/
├── include/
│   ├── core/                 # Memory pools, ring buffers, TSC timing
│   ├── order/                # Order book, price levels, matchers
│   ├── engine/               # Orchestration, risk, sessions
│   ├── market/               # UDP Feed handler
│   ├── net/                  # Sockets, binary codecs
│   ├── persistence/          # Binary Journaling (WAL)
│   └── metrics/              # HDR Histogram latency tracking
├── src/
│   ├── engine/               # Matching engine orchestration
│   ├── order/                # Order book & matching logic
│   ├── net/                  # TCP/UDP I/O implementations
│   └── persistence/          # Journal WAL implementation
├── tests/
│   ├── day4/                 # Loopback networking tests
│   └── day5/                 # Replay & persistence tests
├── benchmarks/
│   ├── nanobench.h           # Microbenchmarking framework
│   └── order_book_bench.cpp  # O(1) scalability tests
├── docs/
│   ├── latency_analysis.md   # Performance breakdown
│   └── Day4_Networking.md    # Networking architecture
├── build_tests.bat           # Integrated MSVC build script
└── CMakeLists.txt            # CMake build configuration
```

## Building the Project

### Method 1: CMake (Recommended)

CMake provides a consistent build experience across platforms.

#### Windows (MSVC)

1. **Open "x64 Native Tools Command Prompt for VS 2022"**:
   - Search for "x64 Native Tools Command Prompt" in Start Menu
   - Or use: `Start > Visual Studio 2022 > x64 Native Tools Command Prompt`

2. **Navigate to project directory**:
   ```bash
   cd path\to\low-latency-project
   ```

3. **Create build directory and configure**:
   ```bash
   mkdir build
   cd build
   cmake .. -G "Visual Studio 17 2022" -A x64
   ```

4. **Build the project**:
   ```bash
   cmake --build . --config Release
   ```

5. **Run the executable**:
   ```bash
   Release\LowLatencyProject.exe
   ```

#### Linux/macOS

```bash
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
./LowLatencyProject
```

### Method 2: Direct Compilation (Simple Projects)

For quick development or small projects, you can compile directly without CMake.

#### Windows (MSVC)

1. **Open "x64 Native Tools Command Prompt for VS 2022"**

2. **Navigate to project directory**:
   ```bash
   cd path\to\low-latency-project
   ```

3. **Compile**:
   ```bash
   cl /std:c++20 /O2 /EHsc /I include src\main.cpp src\lowlatency.cpp /Fe:main.exe /link
   ```

   **Compiler flags explanation**:
   - `/std:c++20` - Use C++20 standard
   - `/O2` - Optimize for speed
   - `/EHsc` - Exception handling
   - `/I include` - Include directory
   - `/Fe:main.exe` - Output executable name
   - `/link` - Link options

4. **Run**:
   ```bash
   main.exe
   ```

#### Linux (GCC)

```bash
g++ -std=c++20 -O3 -I include src/main.cpp src/lowlatency.cpp -o main
./main
```

#### macOS (Clang)

```bash
clang++ -std=c++20 -O3 -I include src/main.cpp src/lowlatency.cpp -o main
./main
```

### Method 3: Using Build Scripts (Windows)

For convenience, an integrated MSVC build script is provided:

- **`build_tests.bat`** - Compiles all core logic, tests, and benchmarks using MSVC.

## Optimization Flags

The project uses compiler-specific optimization flags for low-latency:

### MSVC
- `/O2` - Maximum optimization for speed
- `/Ob2` - Inline function expansion
- `/GL` - Whole program optimization

### GCC/Clang
- `-O3` - Highest optimization level
- `-march=native` - Optimize for current CPU
- `-ffast-math` - Fast floating-point operations
- `-funroll-loops` - Loop unrolling

## CI/CD Integration

Example GitHub Actions workflow for automated building and testing:

```yaml
- name: Build and Test
  run: |
    cmake -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build
    cd build && ctest --output-on-failure
```

## Troubleshooting

### "Cannot open include file 'iostream'"
- Ensure you're using a Developer Command Prompt with environment variables set.
- Check that Visual Studio C++ workload is installed.

### "C++20 not supported"
- Upgrade your compiler:
  - **MSVC**: Visual Studio 2019+ (16.8+)
  - **GCC**: Version 10+
  - **Clang**: Version 12+

## Progress Tracker

- [x] **Day 1**: Lock-free SPSC Ring Buffer & Object Pool.
- [x] **Day 2**: Intrusive Order Book & Price Levels.
- [x] **Day 3**: Branchless Risk Checks & Matching Engine Core.
- [x] **Day 4**: Platform-aware Sockets & Decoupled I/O threads.
- [x] **Day 5**: Binary Journaling (WAL) & HDR Histogram Metrics.

## License

MIT License (c) 2026
