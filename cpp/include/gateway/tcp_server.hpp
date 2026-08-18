#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <vector>

#include <boost/asio.hpp>

namespace gateway {

class TcpSession final : public std::enable_shared_from_this<TcpSession> {
public:
    using tcp = boost::asio::ip::tcp;

    explicit TcpSession(tcp::socket socket);

    void start();

private:
    void read_header();
    void read_body(std::uint32_t packet_size);
    void write_packet();
    void close();

    tcp::socket socket_;
    std::array<std::uint8_t, 6> header_buffer_{};
    std::vector<std::uint8_t> packet_;
};

class TcpServer final {
public:
    using tcp = boost::asio::ip::tcp;

    TcpServer(boost::asio::io_context& io_context, std::uint16_t port);

    void start();

private:
    void accept_next();

    tcp::acceptor acceptor_;
};

}  // namespace gateway
