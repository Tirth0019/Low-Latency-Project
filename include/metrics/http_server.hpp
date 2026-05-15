#pragma once
#include "metrics/latency_tracker.hpp"
#include <atomic>
#include <cstdint>


namespace metrics {

class HttpServer {
private:
  LatencyTracker &tracker_;
  std::atomic<uint64_t> &order_count_;
  std::atomic<bool> running_{true};
  std::uint64_t uptime_sec_{0};

public:
  HttpServer(LatencyTracker &tracker, std::atomic<uint64_t> &order_count)
      : tracker_(tracker), order_count_(order_count) {}

  void run(uint16_t port);
  void stop() { running_.store(false); }
};

} // namespace metrics