# Low Latency Project

A high-performance C++ project focused on low-latency performance optimizations with cross-platform support.

## Features

- **C++20** implementation for modern C++ features
- **Low-latency optimizations** with compiler-specific flags
- **Cross-platform support** (GCC, Clang, MSVC)
- **CMake build system** for easy integration
- Optimized compilation flags for maximum performance

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
low-latency-project/
├── include/
│   ├── core/                 # Core types, memory, time, ring buffers
│   ├── order/                # Order types and book interfaces
│   ├── engine/               # Engine interfaces, sessions, risk checks
│   ├── market/               # Market data interfaces
│   ├── net/                  # Sockets, protocols, codecs
│   ├── persistence/          # Journaling, replay interfaces
│   ├── metrics/              # Telemetry interfaces
│   ├── utils/                # Minimal shared helpers
│   └── lowlatency.h          # Legacy/umbrella header (optional)
├── src/
│   ├── core/                 # Core implementations
│   ├── order/                # Order book + matching implementations
│   ├── engine/               # Engine orchestration
│   ├── market/               # Market data handling
│   ├── net/                  # Networking implementations
│   ├── persistence/          # Journaling/replay implementations
│   ├── metrics/              # Metrics implementations
│   ├── utils/                # Helper implementations
│   ├── main.cpp              # Main entry point
│   └── lowlatency.cpp        # Legacy/utility implementation (optional)
├── tests/                    # Unit/integration tests
├── benchmarks/               # Microbenchmarks and latency tests
├── config/                   # Runtime configuration files
├── scripts/                  # Build/dev scripts
├── docs/                     # Design docs and notes
├── examples/                 # Minimal usage examples
├── build/                    # Build output directory (git-ignored)
├── CMakeLists.txt            # CMake configuration
├── compile_simple.bat        # Simple build script (Windows)
├── build_manual.bat          # Manual build script with MSVC
└── build.bat                 # CMake build script
```

## Building the Project

### Method 1: CMake (Recommended for Production)

CMake provides a consistent build experience across platforms and is ideal for corporate environments.

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

For convenience, pre-configured build scripts are provided:

- **`compile_simple.bat`** - Direct MSVC compilation (no CMake required)
- **`build_manual.bat`** - Manual build with detailed output
- **`build.bat`** - CMake-based build (requires CMake)

Simply double-click any script or run from command prompt.

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

## Corporate Usage Guidelines

### For Development Teams

1. **Standard Build Process**: Use CMake (Method 1) for consistency
2. **Version Control**: The `build/` directory is git-ignored - don't commit build artifacts
3. **Compiler Version**: Ensure all team members use compatible compiler versions
4. **Build Scripts**: Customize build scripts for your CI/CD pipeline

### CI/CD Integration

Example GitHub Actions workflow:
```yaml
- name: Build
  run: |
    mkdir build
    cd build
    cmake .. -G "Visual Studio 17 2022" -A x64
    cmake --build . --config Release
```

### Dependency Management

Currently, this project has no external dependencies. When adding dependencies:
- Use CMake `find_package()` for system libraries
- Consider vcpkg or Conan for package management
- Document all dependencies in this README

## Troubleshooting

### "Cannot open include file 'iostream'"
- Ensure you're using a Developer Command Prompt with environment variables set
- Check that Visual Studio C++ workload is installed
- Verify IntelliSense configuration in `.vscode/c_cpp_properties.json`

### "CMake not found"
- Install CMake from https://cmake.org/download/
- Or use direct compilation (Method 2)
- Or use the provided build scripts (Method 3)

### "C++20 not supported"
- Upgrade your compiler:
  - **MSVC**: Visual Studio 2019+ (16.8+)
  - **GCC**: Version 10+
  - **Clang**: Version 12+

## Contributing

1. Follow C++20 best practices
2. Maintain low-latency optimization focus
3. Test on multiple platforms when possible
4. Update this README with any new build requirements

## License

[Add your license here]
