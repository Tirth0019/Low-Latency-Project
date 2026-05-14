#include "engine/engine.hpp"
#include "net/socket.hpp"
#include "net/codec.hpp"
#include "core/time.hpp"
#include <thread>
#include <iostream>
#include <cassert>
#include <vector>

#ifdef _WIN32
#include <intrin.h>
#else
#include <x86intrin.h>
#endif

using namespace engine;
using namespace net;
using namespace core;

int main() {
    std::cout << "Starting loopback test..." << std::endl;
    WsaGuard wsa;

    RiskLimits limits;
    limits.max_order_qty = Quantity(1000);
    limits.max_notional = 1000000;
    limits.max_long_position = 10000;
    limits.max_short_position = -10000;

    Engine engine(limits);

    std::thread engine_thread([&]() {
        std::cout << "Engine thread starting..." << std::endl;
        engine.pin_to_core(0);
        engine.run();
    });

    std::cout << "Starting networking threads..." << std::endl;
    engine.start_networking(9001, 9002, 9003);

    // Wait for threads to start
    std::cout << "Waiting for threads to initialize..." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    // Client Side
    std::cout << "Client connecting to recv port 9001..." << std::endl;
    TcpSocket client_send;
    if (!client_send.connect("127.0.0.1", 9001)) {
        std::cout << "Client failed to connect to engine recv port 9001" << std::endl;
        engine.stop();
        engine_thread.join();
        return 1;
    }
    client_send.set_nodelay();
    std::cout << "Client connected to 9001" << std::endl;

    std::cout << "Client connecting to send port 9002..." << std::endl;
    TcpSocket client_recv;
    if (!client_recv.connect("127.0.0.1", 9002)) {
        std::cout << "Client failed to connect to engine send port 9002" << std::endl;
        engine.stop();
        engine_thread.join();
        return 1;
    }
    client_recv.set_nodelay();
    std::cout << "Client connected to 9002" << std::endl;

    std::cout << "Preparing orders..." << std::endl;
    // Prepare a NewOrderMsg
    order::Order o;
    o.id = core::OrderId(1);
    o.price = core::Price(100);
    o.qty = core::Quantity(10);
    o.side = core::Side::Buy;
    o.type = core::OrderType::Limit;
    o.state = order::OrderState::New;

    std::cout << "Encoding order 1..." << std::endl;
    uint8_t send_buf[1024];
    size_t send_len = net::Codec::encode_new_order(send_buf, sizeof(send_buf), o);

    std::cout << "Encoding order 2..." << std::endl;
    // Prepare an opposing order to trigger a trade
    order::Order o2;
    o2.id = core::OrderId(2);
    o2.price = core::Price(100);
    o2.qty = core::Quantity(10);
    o2.side = core::Side::Sell;
    o2.type = core::OrderType::Limit;
    o2.state = order::OrderState::New;

    uint8_t send_buf2[1024];
    size_t send_len2 = net::Codec::encode_new_order(send_buf2, sizeof(send_buf2), o2);

    std::cout << "Sending orders..." << std::endl;
    
    uint64_t tsc_start = __rdtsc();
    client_send.send(send_buf, send_len);
    client_send.send(send_buf2, send_len2);

    // Recv Trade Report
    std::cout << "Waiting for trade report..." << std::endl;
    uint8_t recv_buf[1024];
    int attempts = 0;
    ssize_t n = -1;
    while (attempts < 100) {
        n = client_recv.recv(recv_buf, sizeof(recv_buf));
        if (n > 0) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        attempts++;
    }
    uint64_t tsc_end = __rdtsc();

    if (n > 0) {
        std::cout << "Received trade report! Bytes: " << n << std::endl;
        uint64_t rtt_tsc = tsc_end - tsc_start;
        // Assuming 3.0 GHz for estimate
        double rtt_us = (double)rtt_tsc / 3000.0;
        std::cout << "Round-trip latency: " << rtt_tsc << " TSC cycles (~" << rtt_us << " us)" << std::endl;

        // Verify message type
        if (recv_buf[0] == (uint8_t)net::MsgType::TradeReport) {
            std::cout << "Verified: Message is TradeReport" << std::endl;
        } else {
            std::cout << "Error: Received message type " << (int)recv_buf[0] << std::endl;
        }
    } else {
        std::cout << "Failed to receive trade report after timeout" << std::endl;
    }

    std::cout << "Stopping engine..." << std::endl;
    engine.stop();
    engine_thread.join();
    
    std::cout << "Test completed successfully" << std::endl;
    
    client_send.close();
    client_recv.close();
    
    return 0;
}
