#include "engine/engine.hpp"
#include "core/time.hpp"
#include <iostream>

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
  latency_samples_.reserve(100'000);
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

    auto trades = matcher_->match(aggressive);

    // 6. post-fill risk update
    for (auto &t : trades)
      risk_->on_fill(inbound_order.side, t.trade_qty);

    // 7. emit to outbound ring
    for (auto &t : trades)
      outbound_.push(t);

#ifdef _WIN32
    uint64_t tsc_out = __rdtsc(); // ← latency measurement ends here
#else
    uint64_t tsc_out = __rdtsc(); // ← latency measurement ends here
#endif
    latency_samples_.push_back(tsc_out - tsc_in);

    session_.next_seq();
  }
}

void Engine::stop() {
  running_.store(false, std::memory_order_release);
  session_.close();
}

} // namespace engine
