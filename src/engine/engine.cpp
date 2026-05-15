#include "engine/engine.hpp"
#include "core/time.hpp"
#include <iostream>
#include <random>

#ifdef _WIN32
#include <intrin.h>
#include <windows.h>
#else
#include <pthread.h>
#include <sched.h>
#include <x86intrin.h>
#endif

namespace engine {

using namespace core;
using namespace order;

Engine::Engine(RiskLimits limits)
    : book_(std::make_unique<OrderBook>()),
      matcher_(std::make_unique<MatchingEngine>(*book_)), limits_(limits) {

  risk_ = std::make_unique<RiskChecker>(RiskChecker{limits_, risk_state_});
  feed_handler_ = std::make_unique<market::FeedHandler>(inbound_);
  
  double ns_per_tick = core::time::calibrate_tsc_ns();
  tracker_ = std::make_unique<metrics::LatencyTracker>(ns_per_tick);
  
  
  journal_ = std::make_unique<persistence::Journal>();
  journal_->open("engine.wal");

  http_server_ = std::make_unique<metrics::HttpServer>(*tracker_, order_count_);
}

Engine::~Engine() {}

void Engine::pin_to_core(int core_id) {
#ifdef __linux__
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(core_id, &cpuset);
  pthread_setaffinity_np(pthread_self(), sizeof(cpuset), &cpuset);
#elif _WIN32
  SetThreadAffinityMask(GetCurrentThread(), 1ULL << core_id);
#endif
}

void Engine::run() {
  session_.open(1);
  std::cout << "Session 1 Active\n"; // Required verification log
  running_.store(true, std::memory_order_release);

  while (running_.load(std::memory_order_acquire)) {
    uint64_t now = core::time::MonotonicClock::now_ns();

    // 1. heartbeat check (no branch cost — just a comparison)
    if (session_.heartbeat_due(now, 1'000'000'000ULL)) {
      session_.last_heartbeat_ns = now;
      // Day 4: emit heartbeat message to outbound
    }

    // 2. poll inbound ring
    Order inbound_order{};
    if (!inbound_.pop(inbound_order))
      continue; // spin — nothing to do

    // 3. validate session active
    if (!session_.is_active())
      continue;

    // 4. pre-trade risk check
#ifdef _WIN32
    uint64_t tsc_in = __rdtsc(); // ← latency measurement starts here
#else
    uint64_t tsc_in = __rdtsc(); // ← latency measurement starts here
#endif
    if (!risk_->check(inbound_order.side, inbound_order.price,
                      inbound_order.qty)) {
      // rejected — emit reject report (stub for now)
      continue;
    }

    // 5. match or add to book
    // CRITICAL FIX: inbound_order is on the stack. We must allocate from the
    // pool before passing to match() so it can rest safely or be deallocated
    // properly.
    Order *aggressive = book_->pool_->allocate();
    if (!aggressive) {
      continue; // Drop if pool exhausted (in a real system, send reject)
    }
    *aggressive = inbound_order; // copy contents

    // Journal inbound order
    {
      uint8_t wire_buf[128];
      size_t len = net::Codec::encode_new_order(wire_buf, sizeof(wire_buf), inbound_order);
      if (len > 0) journal_->write(wire_buf, (uint16_t)len);
    }

    auto trades = matcher_->match(aggressive);

    // 6. post-fill risk update
    for (auto &t : trades) {
      risk_->on_fill(inbound_order.side, t.trade_qty);
      
      // Journal trade event
      uint8_t trade_buf[128];
      size_t tlen = net::Codec::encode_trade_report(trade_buf, sizeof(trade_buf), t);
      if (tlen > 0) journal_->write(trade_buf, (uint16_t)tlen);
    }

    // 7. emit to outbound ring
    for (auto &t : trades)
      outbound_.push(t);

#ifdef _WIN32
    uint64_t tsc_out = __rdtsc(); // ← latency measurement ends here
#else
    uint64_t tsc_out = __rdtsc(); // ← latency measurement ends here
#endif
    tracker_->record(tsc_out - tsc_in);

    // Periodic checkpoint
    if (++journal_record_count_ % 1000 == 0) {
        journal_->checkpoint();
    }

    session_.next_seq();
    order_count_.fetch_add(1, std::memory_order_relaxed);
  }
}

void Engine::start() {
    // Port 8080 is reserved for HTTP/Metrics Dashboard
    // Order Entry: 9001, Trade Reports: 9002, Market Feed: 9003
    start_networking(9001, 9002, 9003);
    
    engine_thread_ = std::thread(&Engine::run, this);
    http_thread_ = std::thread([this]() {
        http_server_->run(8080);
    });

    // Start synthetic order generator
    synth_thread_ = std::thread([this]() {
        std::mt19937 rng(42);
        std::uniform_int_distribution<> price_dist(99, 101);
        std::uniform_int_distribution<> qty_dist(1, 100);
        std::uniform_int_distribution<> side_dist(0, 1);
        uint64_t id = 0;

        while (running_.load(std::memory_order_acquire)) {
            for (int i = 0; i < 100; ++i) {
                order::Order o{};
                o.id    = core::OrderId{id++};
                o.price = core::Price{(int64_t)price_dist(rng)};
                o.qty   = o.remaining_qty = core::Quantity{(uint64_t)qty_dist(rng)};
                o.side  = side_dist(rng) ? core::Side::Buy : core::Side::Sell;
                o.state = order::OrderState::New;
                o.timestamp_ns = core::time::MonotonicClock::now_ns();
                while (!inbound_.push(o) &&
                       running_.load(std::memory_order_acquire)) {}
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });
}

void Engine::stop() {
  running_.store(false, std::memory_order_release);
  
  if (recv_thread_.joinable()) recv_thread_.join();
  if (send_thread_.joinable()) send_thread_.join();
  if (feed_thread_.joinable()) {
      feed_handler_->stop();
      feed_thread_.join();
  }
  
  http_server_->stop();
  if (http_thread_.joinable()) http_thread_.join();
  if (synth_thread_.joinable()) synth_thread_.join();

  if (engine_thread_.joinable()) engine_thread_.join();

  session_.close();
}

void Engine::start_networking(uint16_t recv_port, uint16_t send_port, uint16_t feed_port) {
    recv_thread_ = std::thread(&Engine::recv_loop, this, recv_port);
    send_thread_ = std::thread(&Engine::send_loop, this, send_port);
    feed_thread_ = std::thread(&market::FeedHandler::run, feed_handler_.get(), feed_port);
}

void Engine::recv_loop(uint16_t port) {
    net::TcpSocket server;
    if (!server.bind(port)) {
        std::cerr << "Recv thread failed to bind to " << port << "\n";
        return;
    }
    server.listen();
    server.set_nonblocking();

    std::cout << "Engine Recv thread listening on " << port << "\n";

    while (running_.load(std::memory_order_acquire)) {
        net::TcpSocket client = server.accept();
        if (!client.valid()) {
            // No client, spin a bit
            for (volatile int i = 0; i < 1000; ++i); 
            continue;
        }

        std::cout << "Client connected to engine\n";
        client.set_nonblocking();
        client.set_nodelay();
        client.set_rcvbuf(4 * 1024 * 1024);

        uint8_t buf[1024];
        while (running_.load(std::memory_order_acquire)) {
            // TODO: recv accumulator for TCP fragmentation
            ssize_t n = client.recv(buf, sizeof(buf));
            if (n == 0) {
                std::cout << "Client disconnected from engine" << std::endl;
                break;
            }
            if (n < 0) {
                // EAGAIN or other error
                continue;
            }

            // Decode directly into pool slot
            order::Order* o = book_->pool_->allocate();
            if (o && net::Codec::decode(buf, (size_t)n, o)) {
                while (!inbound_.push(*o)) {
                    // Spin until space
                }
            } else if (o) {
                book_->pool_->deallocate(o);
                std::cerr << "Codec decode failed or pool allocation failed\n";
            }
        }
    }
}

void Engine::send_loop(uint16_t port) {
    net::TcpSocket server;
    if (!server.bind(port)) {
        std::cerr << "Send thread failed to bind to " << port << "\n";
        return;
    }
    server.listen();
    server.set_nonblocking();

    std::cout << "Engine Send thread listening on " << port << "\n";

    while (running_.load(std::memory_order_acquire)) {
        net::TcpSocket client = server.accept();
        if (!client.valid()) {
            for (volatile int i = 0; i < 1000; ++i);
            continue;
        }

        std::cout << "Client connected to engine send channel\n";
        client.set_nonblocking();
        client.set_nodelay();

        uint8_t buf[1024];
        while (running_.load(std::memory_order_acquire)) {
            order::TradeEvent te;
            if (outbound_.pop(te)) {
                size_t len = net::Codec::encode_trade_report(buf, sizeof(buf), te);
                if (len > 0) {
                    client.send(buf, len);
                }
            }
        }
    }
}

} // namespace engine
