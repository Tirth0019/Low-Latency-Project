#include "net/socket.hpp"
#include <iostream>

#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
#else
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <netinet/tcp.h>
  #include <arpa/inet.h>
  #include <unistd.h>
  #include <fcntl.h>
#endif

namespace net {

bool net::TcpSocket::bind(uint16_t port) {
    if (fd_ == INVALID) {
        fd_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (fd_ == INVALID) {
            std::cout << "Socket creation failed for bind" << std::endl;
            return false;
        }
    }

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (::bind(fd_, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        std::cout << "Socket bind failed for port " << port << std::endl;
        return false;
    }
    return true;
}

bool net::TcpSocket::connect(const char* ip, uint16_t port) {
    if (fd_ == INVALID) {
        fd_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (fd_ == INVALID) {
            std::cout << "Socket creation failed for connect" << std::endl;
            return false;
        }
    }

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip, &addr.sin_addr) <= 0) {
        std::cout << "Invalid IP address: " << ip << std::endl;
        return false;
    }

    if (::connect(fd_, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        std::cout << "Socket connect failed to " << ip << ":" << port << " Error: " << WSAGetLastError() << std::endl;
        return false;
    }
    std::cout << "Socket connected to " << ip << ":" << port << std::endl;
    return true;
}

bool net::TcpSocket::listen(int backlog) {
    if (::listen(fd_, backlog) == -1) {
        return false;
    }
    return true;
}

net::TcpSocket net::TcpSocket::accept() {
    SocketHandle client_fd = ::accept(fd_, nullptr, nullptr);
    return TcpSocket(client_fd);
}

bool net::TcpSocket::set_nonblocking() {
#ifdef _WIN32
    unsigned long mode = 1;
    return ioctlsocket(fd_, FIONBIO, &mode) == 0;
#else
    int flags = fcntl(fd_, F_GETFL, 0);
    if (flags == -1) return false;
    return fcntl(fd_, F_SETFL, flags | O_NONBLOCK) == 0;
#endif
}

bool net::TcpSocket::set_rcvbuf(int size_bytes) {
    return setsockopt(fd_, SOL_SOCKET, SO_RCVBUF, (const char*)&size_bytes, sizeof(size_bytes)) == 0;
}

bool net::TcpSocket::set_nodelay() {
    int one = 1;
    return setsockopt(fd_, IPPROTO_TCP, TCP_NODELAY, (const char*)&one, sizeof(one)) == 0;
}

bool TcpSocket::set_tos(uint8_t tos) {
    int val = tos;
    return setsockopt(fd_, IPPROTO_IP, IP_TOS, (const char*)&val, sizeof(val)) == 0;
}

ssize_t net::TcpSocket::send(const void* buf, size_t len) {
    return ::send(fd_, (const char*)buf, (int)len, 0);
}

ssize_t net::TcpSocket::recv(void* buf, size_t len) {
    return ::recv(fd_, (char*)buf, (int)len, 0);
}

void net::TcpSocket::close() {
    if (fd_ != INVALID) {
#ifdef _WIN32
        closesocket(fd_);
#else
        ::close(fd_);
#endif
        fd_ = INVALID;
    }
}

// UdpSocket Implementation

bool net::UdpSocket::create() {
    if (fd_ == INVALID) {
        fd_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    }
    return fd_ != INVALID;
}

bool net::UdpSocket::bind(uint16_t port) {
    if (!create()) return false;

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    return ::bind(fd_, (struct sockaddr*)&addr, sizeof(addr)) != -1;
}

bool UdpSocket::set_nonblocking() {
    if (!create()) return false;
#ifdef _WIN32
    unsigned long mode = 1;
    return ioctlsocket(fd_, FIONBIO, &mode) == 0;
#else
    int flags = fcntl(fd_, f_GETFL, 0);
    if (flags == -1) return false;
    return fcntl(fd_, F_SETFL, flags | O_NONBLOCK) == 0;
#endif
}

bool net::UdpSocket::set_rcvbuf(int size_bytes) {
    return setsockopt(fd_, SOL_SOCKET, SO_RCVBUF, (const char*)&size_bytes, sizeof(size_bytes)) == 0;
}

ssize_t net::UdpSocket::sendto(const void* buf, size_t len, const char* ip, uint16_t port) {
    if (!create()) return -1;

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &addr.sin_addr);

    return ::sendto(fd_, (const char*)buf, (int)len, 0, (struct sockaddr*)&addr, sizeof(addr));
}

ssize_t net::UdpSocket::recvfrom(void* buf, size_t len, char* out_ip, uint16_t* out_port) {
    if (!create()) return -1;

    sockaddr_in addr;
    int addr_len = sizeof(addr);
    ssize_t n = ::recvfrom(fd_, (char*)buf, (int)len, 0, (struct sockaddr*)&addr, &addr_len);
    
    if (n > 0) {
        if (out_ip) {
            inet_ntop(AF_INET, &addr.sin_addr, out_ip, 16);
        }
        if (out_port) {
            *out_port = ntohs(addr.sin_port);
        }
    }
    return n;
}

void net::UdpSocket::close() {
    if (fd_ != INVALID) {
#ifdef _WIN32
        closesocket(fd_);
#else
        ::close(fd_);
#endif
        fd_ = INVALID;
    }
}

} // namespace net
