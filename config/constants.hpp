#pragma once

#include <cstdint>

namespace config {

inline constexpr std::uint32_t kMaxOrders = 1'000'000;
inline constexpr std::uint32_t kMaxPriceLevels = 65'536;
inline constexpr std::int64_t kPriceScale = 10'000;

} // namespace config
