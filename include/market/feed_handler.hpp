#pragma once

#include "net/socket.hpp"
#include "core/ring_buffer.hpp"
#include "order/order.hpp"
#include <cstdint>
#include <iostream>

namespace market {

struct BBO {
    core::Price    bid_price;
    core::Price    ask_price;
    core::Quantity bid_qty;
    core::Quantity ask_qty;
    uint64_t       seq_num{0};
    uint64_t       timestamp_ns{0};
    bool           valid{false};
};

class FeedHandler {
public:
    explicit FeedHandler(core::RingBuffer<order::Order, 4096>& ring)
        : engine_ring_(ring) {}

    void run(uint16_t port) {
        if (!sock_.bind(port)) {
            std::cerr << "Failed to bind market data socket on " << port << std::endl;
            return;
        }
        sock_.set_nonblocking();

        uint8_t buf[1500];
        while (running_) {
            ssize_t n = sock_.recvfrom(buf, sizeof(buf));
            if (n < 0) {
                // For non-blocking, wait a bit
                for (volatile int i = 0; i < 1000; ++i);
                continue;
            }
            if (n == 0) continue;

            if (n < 8) continue; // Min size for seq_num

            uint64_t seq = *reinterpret_cast<uint64_t*>(buf);
            
            if (expected_seq_ != 0 && seq != expected_seq_) {
                ++gap_count_;
                bbo_.valid = false;
                std::cerr << "Market data gap detected! Expected " << expected_seq_ << " got " << seq << "\n";
            }
            expected_seq_ = seq + 1;

            update_bbo(buf + 8, n - 8);
        }
    }

    void stop() { running_ = false; }

    const BBO& bbo() const { return bbo_; }
    uint64_t gap_count() const { return gap_count_; }

private:
    void update_bbo(const uint8_t* payload, size_t len) {
        if (len < sizeof(core::Price) * 2 + sizeof(core::Quantity) * 2) return;

        const int64_t* p = reinterpret_cast<const int64_t*>(payload);
        const uint64_t* q = reinterpret_cast<const uint64_t*>(payload + 16);

        bbo_.bid_price = core::Price(p[0]);
        bbo_.ask_price = core::Price(p[1]);
        bbo_.bid_qty = core::Quantity(q[0]);
        bbo_.ask_qty = core::Quantity(q[1]);
        bbo_.valid = true;

        // Push synthetic order to engine for Day 4 loopback
        // This simulates reacting to market data
        order::Order synthetic;
        synthetic.id = core::OrderId(999999); // Synthetic ID
        synthetic.price = bbo_.bid_price;
        synthetic.qty = bbo_.bid_qty;
        synthetic.side = core::Side::Buy;
        synthetic.type = core::OrderType::Limit;
        
        while (!engine_ring_.push(synthetic)) {
            // Spin until ring has space
        }
    }

    BBO bbo_;
    net::UdpSocket sock_;
    core::RingBuffer<order::Order, 4096>& engine_ring_;
    uint64_t expected_seq_{0};
    uint64_t gap_count_{0};
    bool running_{true};
};

} // namespace market
