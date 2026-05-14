#include "net/codec.hpp"
#include <iostream>
#include <cassert>
#include <vector>

using namespace net;
using namespace order;
using namespace core;

void test_round_trip() {
    std::cout << "Running test_round_trip..." << std::endl;
    Order o;
    o.id = OrderId(12345);
    o.price = Price(67890);
    o.qty = Quantity(100);
    o.side = Side::Sell;
    o.type = OrderType::Limit;

    uint8_t buf[1024];
    size_t len = Codec::encode_new_order(buf, sizeof(buf), o);
    assert(len == sizeof(NewOrderMsg));

    Order decoded;
    bool success = Codec::decode(buf, len, &decoded);
    assert(success);

    assert(decoded.id == o.id);
    assert(decoded.price == o.price);
    assert(decoded.qty == o.qty);
    assert(decoded.side == o.side);
    assert(decoded.type == o.type);
    std::cout << "test_round_trip passed!" << std::endl;
}

void test_corruption() {
    std::cout << "Running test_corruption..." << std::endl;
    Order o;
    o.id = OrderId(1);
    o.price = Price(100);
    o.qty = Quantity(10);
    o.side = Side::Buy;
    o.type = OrderType::Limit;

    uint8_t buf[1024];
    size_t len = Codec::encode_new_order(buf, sizeof(buf), o);

    // Flip one byte in the payload (not the type or length or checksum itself)
    buf[10] ^= 0xFF;

    Order decoded;
    bool success = Codec::decode(buf, len, &decoded);
    assert(!success); // Should fail due to checksum mismatch
    std::cout << "test_corruption passed!" << std::endl;
}

void test_overflow() {
    std::cout << "Running test_overflow..." << std::endl;
    Order o;
    o.id = OrderId(1);
    o.price = Price(100);
    o.qty = Quantity(10);
    o.side = Side::Buy;
    o.type = OrderType::Limit;

    uint8_t buf[10]; // Too small
    size_t len = Codec::encode_new_order(buf, sizeof(buf), o);
    assert(len == 0); // Should return 0 and not overflow
    std::cout << "test_overflow passed!" << std::endl;
}

int main() {
    test_round_trip();
    test_corruption();
    test_overflow();
    std::cout << "All codec unit tests passed!" << std::endl;
    return 0;
}
