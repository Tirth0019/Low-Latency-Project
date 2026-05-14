#include <iostream>
#include "net/socket.hpp"

int main() {
    net::WsaGuard wsa;
    std::cout << "Low Latency Project Skeleton Initialized with Winsock\n";
    return 0;
}
