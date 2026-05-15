#pragma once

#include <memory>
#include <vector>
#include <atomic>

#include "order/order_book.hpp"
#include "engine/matching_engine.hpp"
#include "engine/session.hpp"
#include "engine/risk.hpp"
#include "core/ring_buffer.hpp"
#include "net/socket.hpp"
#include "net/codec.hpp"
#include "market/feed_handler.hpp"
#include "persistence/journal.hpp"
#include "metrics/latency_tracker.hpp"
#include "metrics/http_server.hpp"
#include <thread>

namespace engine {

class Engine {
public:
    Engine(RiskLimits limits);
    ~Engine();

    void pin_to_core(int core_id);
    void run();
    void start();
    void stop();

    void start_networking(uint16_t recv_port, uint16_t send_port, uint16_t feed_port);

    // Accessors for benchmarking
    core::RingBuffer<order::Order, 4096>& get_inbound_ring() { return inbound_; }
    core::RingBuffer<order::TradeEvent, 4096>& get_outbound_ring() { return outbound_; }
    metrics::LatencyTracker* get_tracker() { return tracker_.get(); }
    order::OrderBook& get_book() { return *book_; }

private:
    std::unique_ptr<order::OrderBook>     book_;
    std::unique_ptr<MatchingEngine>       matcher_;
    std::unique_ptr<RiskChecker>          risk_;
    RiskState                             risk_state_;
    Session                               session_;

    core::RingBuffer<order::Order, 4096>      inbound_;
    core::RingBuffer<order::TradeEvent, 4096> outbound_;

    std::unique_ptr<metrics::LatencyTracker> tracker_;
    std::unique_ptr<persistence::Journal>    journal_;
    uint64_t                                  journal_record_count_{0};

    std::atomic<bool>                     running_{false};
    RiskLimits                            limits_;

    std::unique_ptr<market::FeedHandler>  feed_handler_;
    std::thread                           recv_thread_;
    std::thread                           send_thread_;
    std::thread                           feed_thread_;
    std::thread                           engine_thread_;
    std::thread                           http_thread_;

    std::atomic<uint64_t>                 order_count_{0};
    std::unique_ptr<metrics::HttpServer> http_server_;

    void recv_loop(uint16_t port);
    void send_loop(uint16_t port);
};

} // namespace engine
