#include "metrics/http_server.hpp"
#include "net/socket.hpp"
#include <cstring>
#include <string>
#include <fstream>
#include <sstream>


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

    char buf[1024] = {};
    client.recv(buf, sizeof(buf));

    std::string request(buf);
    std::string response;

    if (request.find("GET /metrics") != std::string::npos) {
      std::string json =
          "{\"p50_ns\":" + std::to_string(tracker_.get_p50()) +
          ",\"p99_ns\":" + std::to_string(tracker_.get_p99()) +
          ",\"p999_ns\":" + std::to_string(tracker_.get_p999()) +
          ",\"orders_per_sec\":" + std::to_string(order_count_.load()) +
          ",\"uptime_seconds\":" + std::to_string(this->uptime_sec_) + "}";

      response = "HTTP/1.1 200 OK\r\n"
                 "Content-Type: application/json\r\n"
                 "Access-Control-Allow-Origin: *\r\n"
                 "Content-Length: " +
                 std::to_string(json.size()) + "\r\n"
                 "\r\n" +
                 json;
    } else {
      // Default to serving dashboard.html for GET / or any other path
      std::ifstream f("docs/dashboard.html");
      if (f.good()) {
        std::stringstream ss;
        ss << f.rdbuf();
        std::string html = ss.str();
        response = "HTTP/1.1 200 OK\r\n"
                   "Content-Type: text/html\r\n"
                   "Content-Length: " +
                   std::to_string(html.size()) + "\r\n"
                   "\r\n" +
                   html;
      } else {
        response = "HTTP/1.1 404 Not Found\r\n"
                   "Content-Length: 0\r\n"
                   "\r\n";
      }
    }

    client.send(response.c_str(), response.size());
  }
}

} // namespace metrics