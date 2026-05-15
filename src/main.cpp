#include "engine/engine.hpp"
#include "net/socket.hpp"
#include <atomic>
#include <csignal>
#include <iostream>
#include <thread>

// Global shutdown flag
std::atomic<bool> g_running{true};

void signal_handler(int) { g_running.store(false, std::memory_order_release); }

int main() {
#ifdef _WIN32
  net::WsaGuard wsa;
#endif

  std::signal(SIGINT, signal_handler);
  std::signal(SIGTERM, signal_handler);

  // Configure risk limits
  engine::RiskLimits limits;
  limits.max_order_qty = core::Quantity{10000};
  limits.max_notional = 1000000;
  limits.max_long_position = 50000;
  limits.max_short_position = -50000;

  // Start engine
  engine::Engine eng(limits);
  eng.start();

  std::cout << "Engine Matching Core started (Order Entry: 9001)...\n";
  std::cout << "Dashboard available on http://localhost:8080\n";

  // Block until SIGINT/SIGTERM
  while (g_running.load(std::memory_order_acquire)) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  eng.stop();
  std::cout << "Engine stopped cleanly.\n";
  return 0;
}