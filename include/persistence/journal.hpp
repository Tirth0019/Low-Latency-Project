#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace persistence {

#pragma pack(push, 1)
struct JournalRecord {
    uint64_t seq_no;
    uint64_t timestamp_ns;
    uint16_t payload_len;
    // payload bytes follow immediately in the file
};
#pragma pack(pop)

class Journal {
    int      fd_{-1};       // raw file descriptor (int on Linux, but using generic int for portability)
    void*    handle_{nullptr}; // void* for HANDLE on Windows
    uint64_t write_pos_{0}; // current append position in the file
    uint64_t seq_{0};       // monotonic record counter
    
    static constexpr size_t PRE_ALLOC_BYTES = 512 * 1024 * 1024; // 512MB
    static constexpr size_t BUFFER_SIZE = 4096;
    
    // Aligned buffer for O_DIRECT / FILE_FLAG_NO_BUFFERING
    alignas(4096) uint8_t write_buf_[BUFFER_SIZE];
    size_t buf_pos_{0};

public:
    Journal() = default;
    ~Journal() { close(); }

    bool open(const char* path);
    void write(const void* payload, uint16_t len);
    void checkpoint(); // Flush current buffer to disk
    void close();

    // Replay interface
    static void replay(const char* path, 
                       std::function<void(const JournalRecord&, const uint8_t*)> cb);

private:
    void flush_buffer();
};

} // namespace persistence
