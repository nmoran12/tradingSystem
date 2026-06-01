#include "matching_engine/MatchingEngine.hpp"
#include "viz/ReplayVisualisationStreamServer.hpp"
#include "viz/ReplayVisualisationWriter.hpp"

#include <gtest/gtest.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <cerrno>
#include <chrono>
#include <cstring>
#include <string>
#include <thread>
#include <vector>

using namespace matching_engine;
using namespace market_data;
using namespace viz;

namespace {

OrderCommand make_new(uint64_t order_id, Side side, int64_t price, uint64_t quantity) {
    OrderCommand command;
    command.type = OrderCommandType::NewOrder;
    command.order_id = order_id;
    command.side = side;
    command.order_type = OrderType::Limit;
    command.price = price;
    command.quantity = quantity;
    command.symbol = "AAPL";
    return command;
}

bool line_has_non_empty_trades(const std::string& line) {
    const auto trades_pos = line.find("\"trades\":");
    if (trades_pos == std::string::npos) {
        return false;
    }
    const auto array_start = line.find('[', trades_pos);
    const auto array_end = line.find(']', array_start);
    if (array_start == std::string::npos || array_end == std::string::npos ||
        array_end <= array_start + 1) {
        return false;
    }
    return true;
}

uint16_t unique_stream_port() {
    return static_cast<uint16_t>(29100u + (static_cast<unsigned>(::getpid()) % 500u));
}

std::vector<std::string> read_sse_payloads(uint16_t port) {
    std::vector<std::string> payloads;
    const int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        return payloads;
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    ::inet_pton(AF_INET, "127.0.0.1", &address.sin_addr);

    timeval recv_timeout{};
    recv_timeout.tv_sec = 5;
    recv_timeout.tv_usec = 0;
    ::setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &recv_timeout, sizeof(recv_timeout));

    for (int attempt = 0; attempt < 400; ++attempt) {
        if (::connect(fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == 0) {
            const std::string request =
                "GET /stream HTTP/1.1\r\nHost: 127.0.0.1\r\nAccept: text/event-stream\r\n\r\n";
            ::write(fd, request.data(), request.size());

            char buffer[8192];
            std::string response;
            for (;;) {
                const auto received = ::recv(fd, buffer, sizeof(buffer), 0);
                if (received < 0) {
                    if (errno == EINTR) {
                        continue;
                    }
                    break;
                }
                if (received == 0) {
                    break;
                }
                response.append(buffer, static_cast<std::size_t>(received));
            }

            std::size_t event_start = 0;
            while ((event_start = response.find("data: ", event_start)) != std::string::npos) {
                event_start += 6;
                const auto line_end = response.find('\n', event_start);
                if (line_end == std::string::npos) {
                    break;
                }
                payloads.push_back(response.substr(event_start, line_end - event_start));
                event_start = line_end + 1;
            }

            ::close(fd);
            return payloads;
        }
        if (errno != ECONNREFUSED) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    ::close(fd);
    return payloads;
}

}  // namespace

TEST(ReplayVisualisationStreamTest, RejectsNonLoopbackHost) {
    EXPECT_THROW(ReplayVisualisationStreamServer("0.0.0.0:9000"), std::runtime_error);
}

TEST(ReplayVisualisationStreamTest, RejectsInvalidEndpoint) {
    EXPECT_THROW(ReplayVisualisationStreamServer("127.0.0.1"), std::runtime_error);
    EXPECT_THROW(ReplayVisualisationStreamServer("127.0.0.1:70000"), std::runtime_error);
    EXPECT_THROW(ReplayVisualisationStreamServer("127.0.0.1:abc"), std::runtime_error);
}

TEST(ReplayVisualisationStreamTest, SendsSseRecordToConnectedClient) {
    const uint16_t port = unique_stream_port();
    std::atomic<bool> server_ready{false};
    std::thread server_thread([&]() {
        ReplayVisualisationStreamServer server("127.0.0.1:" + std::to_string(port));
        server_ready.store(true);
        server.wait_for_client();
        server.send_sse_record("{\"schemaVersion\":1,\"index\":0}");
        server.close();
    });

    while (!server_ready.load()) {
        std::this_thread::yield();
    }

    const auto payloads = read_sse_payloads(port);
    server_thread.join();

    ASSERT_EQ(payloads.size(), 1u);
    EXPECT_NE(payloads[0].find("\"schemaVersion\":1"), std::string::npos);
}

TEST(ReplayVisualisationStreamTest, StreamsTradeRecordWhenOrdersCross) {
    const uint16_t port = unique_stream_port();
    std::vector<std::string> payloads;

    std::thread server_thread([&]() {
        ReplayVisualisationStreamServer server("127.0.0.1:" + std::to_string(port));
        server.wait_for_client();

        MatchingEngine engine;
        std::vector<EngineEvent> scratch;
        scratch.reserve(4);

        engine.process_into(make_new(1, Side::SELL, 100, 10), scratch);
        payloads.push_back(ReplayVisualisationWriter::format_record(0, make_new(1, Side::SELL, 100, 10),
                                                                    scratch, engine.book()));
        server.send_sse_record(payloads.back());

        const auto buy = make_new(2, Side::BUY, 101, 10);
        engine.process_into(buy, scratch);
        payloads.push_back(
            ReplayVisualisationWriter::format_record(1, buy, scratch, engine.book()));
        server.send_sse_record(payloads.back());
        server.close();
    });

    const auto received = read_sse_payloads(port);
    server_thread.join();

    ASSERT_EQ(received.size(), 2u);
    EXPECT_NE(received[0].find("\"schemaVersion\":1"), std::string::npos);
    EXPECT_FALSE(line_has_non_empty_trades(received[0]));
    ASSERT_TRUE(line_has_non_empty_trades(received[1]));
    EXPECT_NE(received[1].find("\"price\":100"), std::string::npos);
}
