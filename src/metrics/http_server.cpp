#include "metrics/http_server.hpp"
#include "net/socket.hpp"
#include <cstring>
#include <string>


namespace metrics {

void HttpServer::run(uint16_t port) {
  net::TcpSocket server;
  if (!server.bind(port)) {
    return;
  }
  
  if (!server.listen(10)) {
    return;
  }

  while (running_.load(std::memory_order_acquire)) {
    net::TcpSocket client = server.accept();
    if (!client.valid()) {
      continue;
    }

    char buf[512] = {};
    client.recv(buf, sizeof(buf));

    std::string json =
        "{\"p50_ns\":" + std::to_string(tracker_.get_p50()) +
        ",\"p99_ns\":" + std::to_string(tracker_.get_p99()) +
        ",\"p999_ns\":" + std::to_string(tracker_.get_p999()) +
        ",\"orders_per_sec\":" + std::to_string(order_count_.load()) +
        ",\"uptime_sec\":" + std::to_string(this->uptime_sec_) + "}";

    std::string response = "HTTP/1.1 200 OK\r\n"
                           "Content-Type: application/json\r\n"
                           "Access-Control-Allow-Origin: *\r\n"
                           "Content-Length: " +
                           std::to_string(json.size()) +
                           "\r\n"
                           "\r\n" +
                           json;

    client.send(response.c_str(), response.size());
  }
}

} // namespace metrics