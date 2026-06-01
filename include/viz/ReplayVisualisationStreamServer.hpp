#pragma once

#include <cstdint>
#include <string>

namespace viz {

// Proof-of-concept localhost-only HTTP/SSE server for replay visualisation.
//
// Limitations:
// - Single client; blocks in wait_for_client() until one connection is accepted.
// - Loopback bind only (127.0.0.1 / localhost).
// - No TLS, auth, backpressure, or graceful shutdown beyond close().
// - Minimal HTTP request handling (discards incoming bytes after connect).
class ReplayVisualisationStreamServer {
public:
    explicit ReplayVisualisationStreamServer(const std::string& host_port);

    ReplayVisualisationStreamServer(const ReplayVisualisationStreamServer&) = delete;
    ReplayVisualisationStreamServer& operator=(const ReplayVisualisationStreamServer&) =
        delete;
    ReplayVisualisationStreamServer(ReplayVisualisationStreamServer&&) = delete;
    ReplayVisualisationStreamServer& operator=(ReplayVisualisationStreamServer&&) = delete;

    ~ReplayVisualisationStreamServer();

    void wait_for_client();
    void send_sse_record(const std::string& json_record);
    void close();

    [[nodiscard]] std::string bind_address() const {
        return bind_address_;
    }

    [[nodiscard]] uint16_t port() const {
        return port_;
    }

private:
    static void parse_host_port(const std::string& host_port, std::string& host_out,
                                uint16_t& port_out);

    std::string bind_address_;
    uint16_t port_ = 0;
    int listen_fd_ = -1;
    int client_fd_ = -1;
};

}  // namespace viz
