cmake_minimum_required(VERSION 3.15)
project(LowLatencyProject VERSION 1.0.0 LANGUAGES CXX)

# -----------------------------
# Language standard
# -----------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# -----------------------------
# Executable
# -----------------------------
add_executable(LowLatencyProject
    src/main.cpp
    src/lowlatency.cpp
    # add more .cpp files here, e.g.:
    # src/order/order_book.cpp
    # src/engine/matching_engine.cpp
)

# -----------------------------
# Include directories
# -----------------------------
target_include_directories(LowLatencyProject
    PRIVATE
        include
        include/core
        include/order
        include/engine
        include/market
        include/net
        include/persistence
        include/metrics
        include/utils
)

# -----------------------------
# Compiler warnings
# -----------------------------
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(LowLatencyProject PRIVATE
        -Wall
        -Wextra
    )
elseif(MSVC)
    target_compile_options(LowLatencyProject PRIVATE
        /W4
    )
endif()

# -----------------------------
# Build-type specific flags
# -----------------------------
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(LowLatencyProject PRIVATE
            -O3
            -march=native
            -mtune=native
        )
    elseif(MSVC)
        target_compile_options(LowLatencyProject PRIVATE
            /O2
            /Ob2
        )
    endif()
elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(LowLatencyProject PRIVATE
            -O2
            -g
        )
    elseif(MSVC)
        target_compile_options(LowLatencyProject PRIVATE
            /O2
            /Zi
        )
    endif()
else()
    # Debug and other non-Release builds
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(LowLatencyProject PRIVATE
            -O0
            -g
        )
    elseif(MSVC)
        target_compile_options(LowLatencyProject PRIVATE
            /Od
            /Zi
        )
    endif()
endif()

# -----------------------------
# Link Time Optimization (Release only)
# -----------------------------
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    include(CheckIPOSupported)
    check_ipo_supported(RESULT ipo_supported OUTPUT error)

    if(ipo_supported)
        set_property(TARGET LowLatencyProject PROPERTY
            INTERPROCEDURAL_OPTIMIZATION TRUE
        )
    endif()
endif()