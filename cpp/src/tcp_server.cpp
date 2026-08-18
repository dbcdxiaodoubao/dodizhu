#include "gateway/tcp_server.hpp"

#include <algorithm>
#include <iostream>

#include "gateway/packet_codec.hpp"

namespace gateway {
namespace {

constexpr std::uint32_t max_packet_size = 64U * 1024U;

}  // namespace

TcpSession::TcpSession(tcp::socket socket) : socket_(std::move(socket)) {
}

void TcpSession::start() {
    read_header();
}

void TcpSession::read_header() {
    auto self = shared_from_this();
    boost::asio::async_read(
        socket_,
        boost::asio::buffer(header_buffer_),
        [self](const boost::system::error_code& error, std::size_t) {
            if (error) {
                self->close();
                return;
            }

            const auto header = PacketCodec::decode_header(self->header_buffer_);
            if (!header || header->packet_size > max_packet_size) {
                self->close();
                return;
            }

            self->read_body(header->packet_size);
        });
}

void TcpSession::read_body(std::uint32_t packet_size) {
    const auto body_size = packet_size - PacketCodec::header_size;
    packet_.resize(packet_size);
    std::copy(header_buffer_.begin(), header_buffer_.end(), packet_.begin());

    if (body_size == 0) {
        write_packet();
        return;
    }

    auto self = shared_from_this();
    boost::asio::async_read(
        socket_,
        boost::asio::buffer(packet_.data() + PacketCodec::header_size, body_size),
        [self](const boost::system::error_code& error, std::size_t) {
            if (error) {
                self->close();
                return;
            }

            self->write_packet();
        });
}

void TcpSession::write_packet() {
    auto self = shared_from_this();
    boost::asio::async_write(
        socket_,
        boost::asio::buffer(packet_),
        [self](const boost::system::error_code& error, std::size_t) {
            if (error) {
                self->close();
                return;
            }

            self->read_header();
        });
}

void TcpSession::close() {
    boost::system::error_code ignored;
    socket_.shutdown(tcp::socket::shutdown_both, ignored);
    socket_.close(ignored);
}

TcpServer::TcpServer(boost::asio::io_context& io_context, std::uint16_t port)
    : acceptor_(io_context, tcp::endpoint(tcp::v4(), port)) {
}

void TcpServer::start() {
    accept_next();
}

void TcpServer::accept_next() {
    acceptor_.async_accept(
        [this](const boost::system::error_code& error, tcp::socket socket) {
            if (!error) {
                std::make_shared<TcpSession>(std::move(socket))->start();
            }

            accept_next();
        });
}

}  // namespace gateway
