#include "viz/ReplayVisualisationStreamServer.hpp"

#include "viz/ReplayVisualisationWriter.hpp"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>

namespace viz {
namespace {

void throw_runtime_error(const std::string& message) {
    throw std::runtime_error(message);
}

void write_all(int fd, const std::string& data) {
    std::size_t offset = 0;
    while (offset < data.size()) {
        const auto written =
            ::write(fd, data.data() + offset, data.size() - offset);
        if (written < 0) {
            if (errno == EINTR) {
                continue;
            }
            throw_runtime_error("stream write failed: " + std::string(std::strerror(errno)));
        }
        if (written == 0) {
            throw_runtime_error("stream write failed: connection closed");
        }
        offset += static_cast<std::size_t>(written);
    }
}

void discard_http_request(int client_fd) {
    char buffer[1024];
    for (;;) {
        const auto received = ::recv(client_fd, buffer, sizeof(buffer), 0);
        if (received <= 0) {
            if (received < 0 && errno == EINTR) {
                continue;
            }
            throw_runtime_error("failed to read HTTP request from stream client");
        }
        std::string_view chunk(buffer, static_cast<std::size_t>(received));
        if (chunk.find("\r\n\r\n") != std::string_view::npos) {
            return;
        }
    }
}

bool is_loopback_host(const std::string& host) {
    return host == "127.0.0.1" || host == "localhost";
}

}  // namespace

void ReplayVisualisationStreamServer::parse_host_port(const std::string& host_port,
                                                      std::string& host_out,
                                                      uint16_t& port_out) {
    const auto colon = host_port.rfind(':');
    if (colon == std::string::npos || colon == 0 || colon + 1 >= host_port.size()) {
        throw_runtime_error(
            "invalid stream endpoint (expected host:port, e.g. 127.0.0.1:9000): " +
            host_port);
    }

    host_out = host_port.substr(0, colon);
    const auto port_string = host_port.substr(colon + 1);

    if (!is_loopback_host(host_out)) {
        throw_runtime_error("stream endpoint must be localhost (127.0.0.1 or localhost): " +
                            host_out);
    }

    try {
        const unsigned long port_value = std::stoul(port_string);
        if (port_value == 0 || port_value > 65535) {
            throw std::out_of_range("port out of range");
        }
        port_out = static_cast<uint16_t>(port_value);
    } catch (const std::exception&) {
        throw_runtime_error("invalid stream port: " + port_string);
    }
}

ReplayVisualisationStreamServer::ReplayVisualisationStreamServer(
    const std::string& host_port) {
    parse_host_port(host_port, bind_address_, port_);

    listen_fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd_ < 0) {
        throw_runtime_error("failed to create stream socket: " +
                            std::string(std::strerror(errno)));
    }

    const int reuse = 1;
    if (::setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        ::close(listen_fd_);
        listen_fd_ = -1;
        throw_runtime_error("failed to set SO_REUSEADDR: " + std::string(std::strerror(errno)));
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(port_);
    if (::inet_pton(AF_INET, "127.0.0.1", &address.sin_addr) != 1) {
        ::close(listen_fd_);
        listen_fd_ = -1;
        throw_runtime_error("failed to configure loopback address");
    }

    if (::bind(listen_fd_, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0) {
        ::close(listen_fd_);
        listen_fd_ = -1;
        throw_runtime_error("failed to bind stream endpoint 127.0.0.1:" +
                            std::to_string(port_) + ": " + std::string(std::strerror(errno)));
    }

    if (::listen(listen_fd_, 1) < 0) {
        ::close(listen_fd_);
        listen_fd_ = -1;
        throw_runtime_error("failed to listen on stream endpoint: " +
                            std::string(std::strerror(errno)));
    }
}

ReplayVisualisationStreamServer::~ReplayVisualisationStreamServer() {
    close();
}

void ReplayVisualisationStreamServer::wait_for_client() {
    if (client_fd_ >= 0) {
        return;
    }

    std::cerr << "Waiting for replay visualisation stream client on http://127.0.0.1:"
              << port_ << " ...\n";

    sockaddr_in client_address{};
    socklen_t client_length = sizeof(client_address);
    client_fd_ = ::accept(listen_fd_, reinterpret_cast<sockaddr*>(&client_address),
                          &client_length);
    if (client_fd_ < 0) {
        throw_runtime_error("failed to accept stream client: " +
                            std::string(std::strerror(errno)));
    }

    discard_http_request(client_fd_);

    static constexpr char kHeaders[] =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/event-stream\r\n"
        "Cache-Control: no-cache\r\n"
        "Connection: keep-alive\r\n"
        "\r\n";
    write_all(client_fd_, kHeaders);
}

void ReplayVisualisationStreamServer::send_sse_record(const std::string& json_record) {
    if (client_fd_ < 0) {
        throw_runtime_error("stream client not connected");
    }
    write_all(client_fd_, ReplayVisualisationWriter::format_sse_frame(json_record));
}

void ReplayVisualisationStreamServer::close() {
    if (client_fd_ >= 0) {
        ::close(client_fd_);
        client_fd_ = -1;
    }
    if (listen_fd_ >= 0) {
        ::close(listen_fd_);
        listen_fd_ = -1;
    }
}

}  // namespace viz
