#include <iostream>
#include <chrono>
#include "lowlatency.h"

int main() {
    std::cout << "Low Latency Project - C++" << std::endl;
    
    // Example: Measure latency
    auto start = std::chrono::high_resolution_clock::now();
    
    // Your low latency code here
    performLowLatencyOperation();
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    
    std::cout << "Operation latency: " << duration.count() << " nanoseconds" << std::endl;
    
    return 0;
}
