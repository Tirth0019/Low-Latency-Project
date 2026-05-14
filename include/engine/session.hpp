#pragma once

#include <cstdint>

namespace engine {

enum class SessionState : uint8_t {
    Closed, 
    Opening, 
    Active, 
    Closing
};

struct Session {
    uint64_t     seq_num{0};          // monotonically increasing, never resets
    uint64_t     last_heartbeat_ns{0};
    uint64_t     session_open_ns{0};
    SessionState state{SessionState::Closed};
    uint32_t     session_id{0};
    uint8_t      _pad[3];

    void open(uint32_t id) {
        session_id = id;
        state = SessionState::Active;
    }

    void close() {
        state = SessionState::Closed;
    }

    // Returns seq_num++ atomically
    uint64_t next_seq() {
        return seq_num++;
    }

    bool heartbeat_due(uint64_t now_ns, uint64_t interval_ns) const {
        return (now_ns - last_heartbeat_ns) >= interval_ns;
    }

    bool is_active() const { 
        return state == SessionState::Active; 
    }
};

} // namespace engine
