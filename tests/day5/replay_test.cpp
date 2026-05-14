#include <iostream>
#include <cassert>
#include <vector>
#include <filesystem>

#include "engine/engine.hpp"
#include "persistence/journal.hpp"
#include "net/codec.hpp"

using namespace engine;
using namespace order;
using namespace core;
using namespace persistence;

void run_live_session(const char* wal_path, RiskLimits& limits, 
                      Price& out_bid, Price& out_ask, size_t& out_size) {
    std::cout << "Step 1: Running live session..." << std::endl;
    
    // Cleanup old WAL
    if (std::filesystem::exists(wal_path)) {
        std::filesystem::remove(wal_path);
    }

    Engine engine(limits);
    std::thread engine_thread([&engine]() { engine.run(); });
    
    // Wait for engine to start
    while (!engine.get_tracker()) { // tracker is initialized in run()? No, in constructor.
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Extra buffer
    
    // Inject 100 orders
    for (int i = 1; i <= 100; ++i) {
        Order o;
        o.id = OrderId(i);
        o.side = (i % 2 == 0) ? Side::Buy : Side::Sell;
        o.price = Price(100 + (i % 10)); // Varying prices 100-109
        o.qty = Quantity(10);
        o.remaining_qty = o.qty;
        o.type = OrderType::Limit;
        
        while (!engine.get_inbound_ring().push(o)) {}
    }

    // Give some time to process
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Record state
    auto& book = engine.get_book();
    out_size = book.size();
    if (book.has_bids()) out_bid = book.get_best_bid().price;
    if (book.has_asks()) out_ask = book.get_best_ask().price;

    std::cout << "Live Session Stats: Size=" << out_size 
              << " BestBid=" << out_bid.value 
              << " BestAsk=" << out_ask.value << std::endl;

    engine.stop();
    if (engine_thread.joinable()) engine_thread.join();
}

void run_replay_session(const char* wal_path, 
                        Price live_bid, Price live_ask, size_t live_size) {
    std::cout << "Step 2: Running replay session from " << wal_path << "..." << std::endl;
    
    OrderBook book;
    MatchingEngine matcher(book);
    size_t record_count = 0;

    Journal::replay(wal_path, [&](const JournalRecord& hdr, const uint8_t* payload) {
        record_count++;
        
        // We only care about NewOrderMsg for book reconstruction in this simple test.
        uint8_t type = payload[0];
        if (type == 0x01) { // NewOrder
            Order* o = book.pool().allocate();
            if (net::Codec::decode(payload, hdr.payload_len, o)) {
                matcher.match(o);
            }
        }
    });

    std::cout << "Replayed " << record_count << " records." << std::endl;

    // Verify state
    assert(book.size() == live_size);
    if (book.has_bids()) assert(book.get_best_bid().price == live_bid);
    if (book.has_asks()) assert(book.get_best_ask().price == live_ask);

    std::cout << "Replay Verification: SUCCESS" << std::endl;
}

int main() {
    // We must initialize Winsock if on Windows because Engine uses it
#ifdef _WIN32
    net::WsaGuard wsa;
#endif

    RiskLimits limits;
    limits.max_order_qty = Quantity(1000);
    limits.max_notional = 1000000;
    limits.max_long_position = 10000;
    limits.max_short_position = -10000;

    Price live_bid(0), live_ask(0);
    size_t live_size = 0;
    const char* wal_path = "engine.wal";

    run_live_session(wal_path, limits, live_bid, live_ask, live_size);
    run_replay_session(wal_path, live_bid, live_ask, live_size);

    return 0;
}
