#define ANKERL_NANOBENCH_IMPLEMENT
#include "nanobench.h"
#include "order/order_book.hpp"
#include "engine/matching_engine.hpp"
#include <iostream>
#include <string>

using namespace core;
using namespace order;
using namespace engine;

int main() {
    std::cout << "Starting Order Book Microbenchmarks..." << std::endl;
    std::cout << "Note: Ensure CPU Turbo is disabled and power plan is set to High Performance for repeatable results.\n" << std::endl;

    ankerl::nanobench::Bench bench;
    bench.warmup(1000).minEpochIterations(10000);

    // 1. Add Order Benchmark
    {
        auto book = std::make_unique<OrderBook>();
        uint64_t id = 0;
        bench.run("add_order_limit_buy", [&] {
            book->add(OrderId{++id}, Side::Buy, Price{100}, Quantity{10});
        });
    }

    // 2. Cancel Order Benchmark
    {
        auto book = std::make_unique<OrderBook>();
        uint64_t id = 0;
        bench.run("cancel_order_best_bid", [&] {
            auto oid = OrderId{++id};
            book->add(oid, Side::Buy, Price{100}, Quantity{10});
            book->cancel(oid);
        });
    }

    // 3. Match Order Benchmark at varying depths
    for (int depth : {10, 100, 1000}) {
        auto book = std::make_unique<OrderBook>();
        uint64_t id = 0;
        
        // Pre-fill book with 'depth' distinct price levels on the Sell side
        for (int i = 0; i < depth; ++i) {
            book->add(OrderId{++id}, Side::Sell, Price{200 + i}, Quantity{100});
        }

        std::string name = "match_sweep_depth_" + std::to_string(depth);
        bench.run(name.c_str(), [&] {
            MatchingEngine m(*book);
            Order agg{};
            agg.id = OrderId{++id};
            agg.side = Side::Buy;
            agg.price = Price{3000}; // Aggressive price to sweep
            agg.qty = Quantity{1};
            agg.remaining_qty = Quantity{1};
            m.match(&agg);
        });
    }

    return 0;
}
