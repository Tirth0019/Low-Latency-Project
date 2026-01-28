#pragma once

#include <chrono>
#include "core/types.hpp"

namespace core {

inline Timestamp now_ns() {
    const auto now = std::chrono::high_resolution_clock::now();
    const auto duration = now.time_since_epoch();
    return static_cast<Timestamp>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count()
    );
}

inline Timestamp duration_ns(Timestamp start_ns, Timestamp end_ns) {
    return end_ns - start_ns;
}

} // namespace core