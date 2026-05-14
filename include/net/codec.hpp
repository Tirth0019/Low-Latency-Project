#pragma once

#include <cstdint>
#include <cstring>
#include "order/order.hpp"
#include "order/price_level.hpp"

namespace net {

enum class MsgType : uint8_t {
    NewOrder    = 0x01,
    CancelOrder = 0x02,
    TradeReport = 0x03,
    Heartbeat   = 0x04,
    Reject      = 0x05,
};

#pragma pack(push, 1)
struct NewOrderMsg {
    uint8_t  msg_type{(uint8_t)MsgType::NewOrder};
    uint16_t length{sizeof(NewOrderMsg)};
    uint64_t order_id;
    int64_t  price;
    uint64_t qty;
    uint8_t  side;   // 0=Buy 1=Sell
    uint8_t  type;   // 0=Limit 1=Market
    uint32_t checksum;
};

struct TradeReportMsg {
    uint8_t  msg_type{(uint8_t)MsgType::TradeReport};
    uint16_t length{sizeof(TradeReportMsg)};
    uint64_t aggressive_id;
    uint64_t passive_id;
    int64_t  trade_price;
    uint64_t trade_qty;
    uint64_t timestamp_ns;
    uint32_t checksum;
};
#pragma pack(pop)

class Codec {
public:
    static uint32_t calculate_checksum(const uint8_t* data, size_t len) {
        uint32_t crc = 0;
        for (size_t i = 0; i < len; ++i)
            crc ^= ((uint32_t)data[i] << (8 * (i % 4)));
        return crc;
    }

    static size_t encode_new_order(uint8_t* buf, size_t buf_len, const order::Order& o) {
        if (buf_len < sizeof(NewOrderMsg)) return 0;
        NewOrderMsg* msg = reinterpret_cast<NewOrderMsg*>(buf);
        msg->msg_type = (uint8_t)MsgType::NewOrder;
        msg->length = sizeof(NewOrderMsg);
        msg->order_id = o.id.value;
        msg->price = o.price.value;
        msg->qty = o.qty.value;
        msg->side = (uint8_t)o.side;
        msg->type = (uint8_t)o.type;
        
        // Checksum calculation (excluding the checksum field itself)
        msg->checksum = calculate_checksum(buf, sizeof(NewOrderMsg) - 4);
        return sizeof(NewOrderMsg);
    }

    static bool decode(const uint8_t* buf, size_t len, order::Order* out) {
        if (len < 3) return false; // Min length for type + length
        uint8_t type = buf[0];
        uint16_t msg_len = *reinterpret_cast<const uint16_t*>(buf + 1);
        
        if (len < msg_len) return false;

        uint32_t received_checksum = *reinterpret_cast<const uint32_t*>(buf + msg_len - 4);
        uint32_t computed_checksum = calculate_checksum(buf, msg_len - 4);

        if (received_checksum != computed_checksum) {
            return false;
        }

        if (type == (uint8_t)MsgType::NewOrder) {
            const NewOrderMsg* msg = reinterpret_cast<const NewOrderMsg*>(buf);
            out->id = core::OrderId(msg->order_id);
            out->price = core::Price(msg->price);
            out->qty = core::Quantity(msg->qty);
            out->remaining_qty = out->qty;
            out->side = (core::Side)msg->side;
            out->type = (core::OrderType)msg->type;
            out->state = order::OrderState::New;
            return true;
        }
        
        return false;
    }

    static size_t encode_trade_report(uint8_t* buf, size_t buf_len, const order::TradeEvent& te) {
        if (buf_len < sizeof(TradeReportMsg)) return 0;
        TradeReportMsg* msg = reinterpret_cast<TradeReportMsg*>(buf);
        msg->msg_type = (uint8_t)MsgType::TradeReport;
        msg->length = sizeof(TradeReportMsg);
        msg->aggressive_id = te.aggressive_id.value;
        msg->passive_id = te.passive_id.value;
        msg->trade_price = te.trade_price.value;
        msg->trade_qty = te.trade_qty.value;
        msg->timestamp_ns = te.timestamp_ns;
        
        msg->checksum = calculate_checksum(buf, sizeof(TradeReportMsg) - 4);
        return sizeof(TradeReportMsg);
    }
};

} // namespace net
