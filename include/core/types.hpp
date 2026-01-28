#pragma once

#include <cstdint>

namespace core {

using OrderId = std::uint64_t;
using Price = std::int64_t;
using Quantity = std::uint64_t;
using Timestamp = std::uint64_t;

enum class Side : std::uint8_t {
    Buy = 0,
    Sell = 1
};

enum class OrderType : std::uint8_t {
    Limit = 0,
    Market = 1
};

} // namespace core