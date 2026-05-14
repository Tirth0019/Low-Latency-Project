#include <iostream>
#include <cstdlib>
#include <memory>
#include "engine/matching_engine.hpp"

#undef assert
#define assert(condition) \
    do { \
        if (!(condition)) { \
            std::cerr << "Assertion failed: " #condition ", file " __FILE__ ", line " << __LINE__ << std::endl; \
            std::exit(1); \
        } \
    } while (false)

using namespace core;
using namespace order;
using namespace engine;

// Helper to quickly assert sizes
#define ASSERT_BOOK_SIZE(engine, expected) \
    assert(engine->get_book().size() == expected)

void test_1_add_bids() {
    std::cout << "Test 1: Add 3 bids, check best_bid()\n";
    auto book = std::make_unique<OrderBook>();
    auto engine = std::make_unique<MatchingEngine>(*book);
    
    engine->process_new_order(OrderId(1), Side::Buy, Price(100), Quantity(10));
    ASSERT_BOOK_SIZE(engine, 1);
    
    engine->process_new_order(OrderId(2), Side::Buy, Price(105), Quantity(10)); // New best bid
    ASSERT_BOOK_SIZE(engine, 2);
    
    engine->process_new_order(OrderId(3), Side::Buy, Price(102), Quantity(10));
    ASSERT_BOOK_SIZE(engine, 3);
    
    assert(engine->get_book().has_bids());
    assert(engine->get_book().get_best_bid().price == Price(105));
    std::cout << "  Passed\n";
}

void test_2_add_asks() {
    std::cout << "Test 2: Add 2 asks, check best_ask()\n";
    auto book = std::make_unique<OrderBook>();
    auto engine = std::make_unique<MatchingEngine>(*book);
    
    engine->process_new_order(OrderId(1), Side::Sell, Price(200), Quantity(10));
    ASSERT_BOOK_SIZE(engine, 1);
    
    engine->process_new_order(OrderId(2), Side::Sell, Price(195), Quantity(10)); // New best ask
    ASSERT_BOOK_SIZE(engine, 2);
    
    assert(engine->get_book().has_asks());
    assert(engine->get_book().get_best_ask().price == Price(195));
    std::cout << "  Passed\n";
}

void test_3_cancel_best_bid() {
    std::cout << "Test 3: Cancel best bid - verify best_bid() updates\n";
    auto book = std::make_unique<OrderBook>();
    auto engine = std::make_unique<MatchingEngine>(*book);
    
    engine->process_new_order(OrderId(1), Side::Buy, Price(100), Quantity(10));
    engine->process_new_order(OrderId(2), Side::Buy, Price(105), Quantity(10)); // Best
    ASSERT_BOOK_SIZE(engine, 2);
    
    bool cancelled = engine->process_cancel_order(OrderId(2));
    assert(cancelled);
    ASSERT_BOOK_SIZE(engine, 1);
    
    assert(engine->get_book().get_best_bid().price == Price(100));
    std::cout << "  Passed\n";
}

void test_4_full_fill() {
    std::cout << "Test 4: Full fill - aggressive sweeps passive\n";
    auto book = std::make_unique<OrderBook>();
    auto engine = std::make_unique<MatchingEngine>(*book);
    
    // Passive
    engine->process_new_order(OrderId(1), Side::Sell, Price(200), Quantity(10));
    ASSERT_BOOK_SIZE(engine, 1);
    
    // Aggressive
    auto trades = engine->process_new_order(OrderId(2), Side::Buy, Price(205), Quantity(10));
    
    // Both filled
    assert(trades.size() == 1);
    assert(trades[0].trade_qty == Quantity(10));
    assert(trades[0].trade_price == Price(200));
    
    // Neither should be resting
    ASSERT_BOOK_SIZE(engine, 0);
    assert(!engine->get_book().has_asks());
    std::cout << "  Passed\n";
}

void test_5_partial_fill_aggressive_smaller() {
    std::cout << "Test 5: Partial fill - aggressive qty < passive qty\n";
    auto book = std::make_unique<OrderBook>();
    auto engine = std::make_unique<MatchingEngine>(*book);
    
    // Passive
    engine->process_new_order(OrderId(1), Side::Sell, Price(200), Quantity(20));
    ASSERT_BOOK_SIZE(engine, 1);
    
    // Aggressive
    auto trades = engine->process_new_order(OrderId(2), Side::Buy, Price(200), Quantity(5));
    
    assert(trades.size() == 1);
    assert(trades[0].trade_qty == Quantity(5));
    
    // Passive rests with 15
    ASSERT_BOOK_SIZE(engine, 1);
    assert(engine->get_book().get_best_ask().total_qty == 15);
    std::cout << "  Passed\n";
}

void test_6_partial_fill_aggressive_larger() {
    std::cout << "Test 6: Partial fill - aggressive qty > passive qty\n";
    auto book = std::make_unique<OrderBook>();
    auto engine = std::make_unique<MatchingEngine>(*book);
    
    // Passive
    engine->process_new_order(OrderId(1), Side::Sell, Price(200), Quantity(10));
    ASSERT_BOOK_SIZE(engine, 1);
    
    // Aggressive
    auto trades = engine->process_new_order(OrderId(2), Side::Buy, Price(200), Quantity(25));
    
    assert(trades.size() == 1);
    assert(trades[0].trade_qty == Quantity(10));
    
    // Aggressive remainder (15) rests
    ASSERT_BOOK_SIZE(engine, 1);
    assert(engine->get_book().get_best_bid().price == Price(200));
    assert(engine->get_book().get_best_bid().total_qty == 15);
    std::cout << "  Passed\n";
}

void test_7_no_cross() {
    std::cout << "Test 7: No cross - bid price < ask price\n";
    auto book = std::make_unique<OrderBook>();
    auto engine = std::make_unique<MatchingEngine>(*book);
    
    engine->process_new_order(OrderId(1), Side::Sell, Price(200), Quantity(10));
    engine->process_new_order(OrderId(2), Side::Buy, Price(190), Quantity(10));
    
    ASSERT_BOOK_SIZE(engine, 2);
    assert(engine->get_book().has_asks() && engine->get_book().has_bids());
    std::cout << "  Passed\n";
}

void test_8_pool_exhaustion() {
    std::cout << "Test 8: Pool exhaustion - add 65537 orders\n";
    auto book = std::make_unique<OrderBook>();
    auto engine = std::make_unique<MatchingEngine>(*book);
    
    // Add 65536 distinct price levels
    for (int i = 0; i < 65536; ++i) {
        auto trades = engine->process_new_order(OrderId(i + 1), Side::Buy, Price(i + 1), Quantity(1));
        assert(trades.empty());
    }
    
    ASSERT_BOOK_SIZE(engine, 65536);
    
    // The 65537th should fail gracefully
    auto trades = engine->process_new_order(OrderId(65537), Side::Buy, Price(65537), Quantity(1));
    assert(trades.empty());
    ASSERT_BOOK_SIZE(engine, 65536); // Should not have increased
    
    std::cout << "  Passed\n";
}

int main() {
    test_1_add_bids();
    test_2_add_asks();
    test_3_cancel_best_bid();
    test_4_full_fill();
    test_5_partial_fill_aggressive_smaller();
    test_6_partial_fill_aggressive_larger();
    test_7_no_cross();
    test_8_pool_exhaustion();
    
    std::cout << "\nAll 8 tests passed successfully!\n";
    return 0;
}
