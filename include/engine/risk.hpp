#pragma once

#include <cstdint>
#include <unordered_set>
#include "core/types.hpp"

namespace engine {

struct RiskLimits {
    core::Quantity max_order_qty;       // single order size cap
    uint64_t       max_notional;        // price * qty cap per order
    int64_t        max_long_position;   // max net long across all fills
    int64_t        max_short_position;  // max net short (stored as negative)
};

struct RiskState {
    int64_t  net_position{0};     // updated on every fill
    uint64_t orders_sent{0};
    uint64_t orders_rejected{0};
    std::unordered_set<uint32_t> active_client_ids_; // TODO: self-trade check for Day 4
};

struct RiskChecker {
    const RiskLimits& limits_;
    RiskState&        state_;

    // Returns true if order PASSES all risk checks
    bool check(core::Side side, core::Price price, core::Quantity qty) const noexcept {
        uint64_t notional = (uint64_t)price.value * (uint64_t)qty.value;

        // compute all violation flags — branchless
        bool qty_violation      = (qty.value > limits_.max_order_qty.value);
        bool notional_violation = (notional > limits_.max_notional);

        int64_t projected = state_.net_position +
            (side == core::Side::Buy ? (int64_t)qty.value
                                     : -(int64_t)qty.value);
        bool long_violation  = (projected > limits_.max_long_position);
        bool short_violation = (projected < limits_.max_short_position);

        // single OR — no branching
        bool rejected = qty_violation | notional_violation
                      | long_violation | short_violation;

        state_.orders_rejected += rejected;   // branchless counter
        return !rejected;
    }

    // Call after every fill to update position
    void on_fill(core::Side side, core::Quantity qty) noexcept {
        int64_t delta = (side == core::Side::Buy)
                        ? (int64_t)qty.value
                        : -(int64_t)qty.value;
        state_.net_position += delta;
    }
};

} // namespace engine
