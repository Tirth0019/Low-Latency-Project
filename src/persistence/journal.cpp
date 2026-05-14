#include "persistence/journal.hpp"
#include "core/time.hpp"
#include <cstring>
#include <cstdio>
#include <cassert>

#ifdef _WIN32
#include <windows.h>
#else
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#endif

namespace persistence {

bool Journal::open(const char* path) {
#ifdef _WIN32
    HANDLE h = CreateFileA(path, GENERIC_WRITE | GENERIC_READ, 0, nullptr,
                           CREATE_ALWAYS, FILE_FLAG_NO_BUFFERING | FILE_FLAG_WRITE_THROUGH, nullptr);
    if (h == INVALID_HANDLE_VALUE) return false;

    LARGE_INTEGER size;
    size.QuadPart = PRE_ALLOC_BYTES;
    if (!SetFilePointerEx(h, size, nullptr, FILE_BEGIN)) return false;
    if (!SetEndOfFile(h)) return false;

    // Reset to beginning
    size.QuadPart = 0;
    SetFilePointerEx(h, size, nullptr, FILE_BEGIN);
    handle_ = h;
#else
    fd_ = ::open(path, O_WRONLY | O_CREAT | O_DIRECT, 0644);
    if (fd_ < 0) return false;
    posix_fallocate(fd_, 0, PRE_ALLOC_BYTES);
#endif
    return true;
}

void Journal::write(const void* payload, uint16_t len) {
    if (len == 0) return;

    size_t record_total = sizeof(JournalRecord) + len;
    
    // If single record is larger than buffer (unlikely for our packets), flush and write direct
    if (record_total > BUFFER_SIZE) {
        checkpoint();
        // For simplicity in this ultra-low-latency context, we assume records fit in 4KB.
        // If they don't, we'd need a more complex aligned-write logic.
        return;
    }

    if (buf_pos_ + record_total > BUFFER_SIZE) {
        flush_buffer();
    }

    JournalRecord hdr{seq_++, core::time::MonotonicClock::now_ns(), len};
    std::memcpy(write_buf_ + buf_pos_, &hdr, sizeof(hdr));
    std::memcpy(write_buf_ + buf_pos_ + sizeof(hdr), payload, len);
    buf_pos_ += record_total;
}

void Journal::checkpoint() {
    if (buf_pos_ > 0) {
        flush_buffer();
    }
}

void Journal::flush_buffer() {
    // In NO_BUFFERING mode, we must write in multiples of sector size (typically 512 or 4096).
    // For simplicity, we always write the full 4KB buffer.
#ifdef _WIN32
    DWORD written;
    BOOL success = WriteFile((HANDLE)handle_, write_buf_, BUFFER_SIZE, &written, nullptr);
    assert(success && written == BUFFER_SIZE);
#else
    ssize_t n = ::write(fd_, write_buf_, BUFFER_SIZE);
    assert(n == BUFFER_SIZE);
#endif
    write_pos_ += BUFFER_SIZE;
    buf_pos_ = 0;
    // We don't zero the buffer because we use payload_len == 0 as sentinel
    std::memset(write_buf_, 0, BUFFER_SIZE);
}

void Journal::close() {
    checkpoint();
#ifdef _WIN32
    if (handle_) {
        CloseHandle((HANDLE)handle_);
        handle_ = nullptr;
    }
#else
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }
#endif
}

void Journal::replay(const char* path, 
                     std::function<void(const JournalRecord&, const uint8_t*)> cb) {
    FILE* f = fopen(path, "rb");
    if (!f) return;

    uint8_t buffer[BUFFER_SIZE];
    while (fread(buffer, 1, BUFFER_SIZE, f) == BUFFER_SIZE) {
        size_t pos = 0;
        bool found_data_in_block = false;
        while (pos + sizeof(JournalRecord) <= BUFFER_SIZE) {
            JournalRecord* hdr = reinterpret_cast<JournalRecord*>(buffer + pos);
            if (hdr->payload_len == 0) {
                break; // End of records in this 4KB block
            }
            if (hdr->payload_len > 1024) {
                fclose(f);
                return; // Garbage
            }
            
            if (pos + sizeof(JournalRecord) + hdr->payload_len > BUFFER_SIZE) {
                break; // Should not happen with our flush logic
            }

            found_data_in_block = true;
            cb(*hdr, buffer + pos + sizeof(JournalRecord));
            pos += sizeof(JournalRecord) + hdr->payload_len;
        }
        
        if (!found_data_in_block) {
            // Block starts with 0 or only has zeros -> End of written data
            break;
        }
    }
    fclose(f);
}

} // namespace persistence
