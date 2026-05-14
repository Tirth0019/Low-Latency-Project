#pragma once

#include <memory>
#include <vector>
#include <atomic>

#include "order/order_book.hpp"
#include "engine/matching_engine.hpp"
#include "engine/session.hpp"
#include "engine/risk.hpp"
#include "core/ring_buffer.hpp"

namespace engine {

class Engine {
public:
    Engine(RiskLimits limits);
    ~Engine();

    void pin_to_core(int core_id);
    void run();
    void stop();

    // Accessors for benchmarking
    core::RingBuffer<order::Order, 4096>& get_inbound_ring() { return inbound_; }
    core::RingBuffer<order::TradeEvent, 4096>& get_outbound_ring() { return outbound_; }
    std::vector<uint64_t>& get_latency_samples() { return latency_samples_; }

private:
    std::unique_ptr<order::OrderBook>     book_;
    std::unique_ptr<MatchingEngine>       matcher_;
    std::unique_ptr<RiskChecker>          risk_;
    RiskState                             risk_state_;
    Session                               session_;

    core::RingBuffer<order::Order, 4096>      inbound_;
    core::RingBuffer<order::TradeEvent, 4096> outbound_;

    std::vector<uint64_t>                 latency_samples_;
    std::atomic<bool>                     running_{false};
    RiskLimits                            limits_;
};

} // namespace engine
