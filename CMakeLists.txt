cmake_minimum_required(VERSION 3.15)
project(LowLatencyProject VERSION 1.0.0 LANGUAGES CXX)

# -----------------------------
# Language standard
# -----------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# -----------------------------
# Dependencies
# -----------------------------
find_package(Threads REQUIRED)

# -----------------------------
# Core Library
# -----------------------------
add_library(LowLatencyLib STATIC
    src/order/order_book.cpp
    src/engine/engine.cpp
    src/engine/matching_engine.cpp
    src/net/socket.cpp
    src/persistence/journal.cpp
    src/metrics/http_server.cpp
)

target_link_libraries(LowLatencyLib PUBLIC Threads::Threads)

target_include_directories(LowLatencyLib
    PUBLIC
        include
        include/core
        include/order
        include/engine
        include/market
        include/net
        include/persistence
        include/metrics
        include/utils
        config
)

# -----------------------------
# Main Executable
# -----------------------------
add_executable(LowLatencyProject
    src/main.cpp
    src/lowlatency.cpp
)
target_link_libraries(LowLatencyProject PRIVATE LowLatencyLib)

# -----------------------------
# Tests & Benchmarks
# -----------------------------
add_executable(test_ring_buffer tests/test_ring_buffer.cpp)
target_link_libraries(test_ring_buffer PRIVATE LowLatencyLib)

add_executable(order_book_test tests/order_book_test.cpp)
target_link_libraries(order_book_test PRIVATE LowLatencyLib)

add_executable(day3_benchmark src/demo/synthetic_feed.cpp)
target_link_libraries(day3_benchmark PRIVATE LowLatencyLib)

add_executable(loopback_test tests/day4/loopback_test.cpp)
target_link_libraries(loopback_test PRIVATE LowLatencyLib)

add_executable(codec_test tests/codec_test.cpp)
target_link_libraries(codec_test PRIVATE LowLatencyLib)

# -----------------------------
# Compiler warnings & Flags
# -----------------------------
function(set_project_flags target)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${target} PRIVATE -Wall -Wextra)
        if(CMAKE_BUILD_TYPE STREQUAL "Release")
            target_compile_options(${target} PRIVATE -O3 -march=native -mtune=native)
        elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
            target_compile_options(${target} PRIVATE -O2 -g)
        else()
            target_compile_options(${target} PRIVATE -O0 -g)
        endif()
    elseif(MSVC)
        target_compile_options(${target} PRIVATE /W4)
        if(CMAKE_BUILD_TYPE STREQUAL "Release")
            target_compile_options(${target} PRIVATE /O2 /Ob2)
        elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
            target_compile_options(${target} PRIVATE /O2 /Zi)
        else()
            target_compile_options(${target} PRIVATE /Od /Zi)
        endif()
    endif()

    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        include(CheckIPOSupported)
        check_ipo_supported(RESULT ipo_supported OUTPUT error)
        if(ipo_supported)
            set_property(TARGET ${target} PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
        endif()
    endif()
endfunction()

set_project_flags(LowLatencyLib)
set_project_flags(LowLatencyProject)
set_project_flags(test_ring_buffer)
set_project_flags(order_book_test)
set_project_flags(day3_benchmark)
set_project_flags(loopback_test)
set_project_flags(codec_test)