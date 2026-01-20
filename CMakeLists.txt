cmake_minimum_required(VERSION 3.15)
project(LowLatencyProject VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard to C++20 for modern features
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler-specific flags for low latency optimization
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    # Optimize for performance and low latency
    add_compile_options(-O3 -march=native -mtune=native)
    # Additional optimizations for low latency
    add_compile_options(-ffast-math -funroll-loops)
    # Link time optimization
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
elseif(MSVC)
    # MSVC optimizations
    add_compile_options(/O2 /Ob2 /GL)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
endif()

# Include directories
include_directories(include)

# Source files
set(SOURCES
    src/main.cpp
    src/lowlatency.cpp
)

# Header files
set(HEADERS
    include/lowlatency.h
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Enable link time optimization if available
if(CMAKE_INTERPROCEDURAL_OPTIMIZATION_AVAILABLE)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        INTERPROCEDURAL_OPTIMIZATION TRUE
    )
endif()