#pragma once

#include <cstdint>
#include <cstddef>
#include <functional>

namespace core {

// Strong types using a struct wrapper to prevent implicit conversions
template<typename T, typename Tag>
struct StrongType {
    T value;

    constexpr explicit StrongType(T v) : value(v) {}
    constexpr StrongType() : value(0) {}

    constexpr operator T() const { return value; }
    
    constexpr bool operator==(const StrongType& other) const { return value == other.value; }
    constexpr bool operator!=(const StrongType& other) const { return value != other.value; }
    constexpr bool operator<(const StrongType& other) const { return value < other.value; }
    constexpr bool operator>(const StrongType& other) const { return value > other.value; }
    constexpr bool operator<=(const StrongType& other) const { return value <= other.value; }
    constexpr bool operator>=(const StrongType& other) const { return value >= other.value; }
};

struct OrderIdTag {};
struct PriceTag {};
struct QuantityTag {};

using OrderId = StrongType<std::uint64_t, OrderIdTag>;
using Price = StrongType<std::int64_t, PriceTag>;
using Quantity = StrongType<std::uint64_t, QuantityTag>;

// Explicit timestamps can remain uint64_t for ease of math, or also be a strong type. 
// Standard practice usually wraps this if needed, but we keep it simple for now.
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

namespace std {
    template <typename T, typename Tag>
    struct hash<core::StrongType<T, Tag>> {
        std::size_t operator()(const core::StrongType<T, Tag>& st) const noexcept {
            return std::hash<T>()(st.value);
        }
    };
} // namespace std