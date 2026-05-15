#pragma once

#include <cstdint>
#include <cstdio>
#include <cstring>

#ifdef _WIN32
  #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
  #endif
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #pragma comment(lib, "ws2_32.lib")
  using SocketHandle = SOCKET;
  using ssize_t = long long;
  using socklen_t = int;
  constexpr SocketHandle INVALID = INVALID_SOCKET;
#else
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <netinet/tcp.h>
  #include <arpa/inet.h>
  #include <unistd.h>
  #include <fcntl.h>
  using SocketHandle = int;
  constexpr SocketHandle INVALID = -1;
#endif

namespace net {

// RAII wrapper for Winsock initialization on Windows
struct WsaGuard {
    WsaGuard() {
#ifdef _WIN32
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            fprintf(stderr, "WSAStartup failed\n");
        } else {
            printf("WSAStartup successful\n");
        }
#endif
    }
    ~WsaGuard() {
#ifdef _WIN32
        WSACleanup();
#endif
    }
    WsaGuard(const WsaGuard&) = delete;
    WsaGuard& operator=(const WsaGuard&) = delete;
};

class TcpSocket {
    SocketHandle fd_{INVALID};
public:
    TcpSocket() = default;
    explicit TcpSocket(SocketHandle fd) : fd_(fd) {}
    ~TcpSocket() { close(); }

    TcpSocket(const TcpSocket&) = delete;
    TcpSocket& operator=(const TcpSocket&) = delete;

    TcpSocket(TcpSocket&& other) noexcept : fd_(other.fd_) {
        other.fd_ = INVALID;
    }

    TcpSocket& operator=(TcpSocket&& other) noexcept {
        if (this != &other) {
            close();
            fd_ = other.fd_;
            other.fd_ = INVALID;
        }
        return *this;
    }

    bool bind(uint16_t port);
    bool connect(const char* ip, uint16_t port);
    bool listen(int backlog = 5);
    TcpSocket accept();
    bool set_nonblocking();
    bool set_rcvbuf(int size_bytes);
    bool set_nodelay();
    bool set_tos(uint8_t tos);
    ssize_t send(const void* buf, size_t len);
    ssize_t recv(void* buf, size_t len);
    bool valid() const { return fd_ != INVALID; }
    void close();
};

class UdpSocket {
    SocketHandle fd_{INVALID};
public:
    UdpSocket() = default;
    ~UdpSocket() { close(); }

    UdpSocket(const UdpSocket&) = delete;
    UdpSocket& operator=(const UdpSocket&) = delete;

    bool bind(uint16_t port);
    bool set_nonblocking();
    bool set_rcvbuf(int size_bytes);
    ssize_t sendto(const void* buf, size_t len, const char* ip, uint16_t port);
    ssize_t recvfrom(void* buf, size_t len, char* out_ip = nullptr, uint16_t* out_port = nullptr);
    bool valid() const { return fd_ != INVALID; }
    bool create();
    void close();
    // TODO: IP_ADD_MEMBERSHIP for multicast
};

} // namespace net
