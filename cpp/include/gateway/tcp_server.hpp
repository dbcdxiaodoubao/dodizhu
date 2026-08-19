#pragma once

#include <array>
#include <cstdint>
#include <deque>
#include <memory>
#include <string>
#include <vector>

#include <boost/asio.hpp>

#include "gateway/backend_client.hpp"
#include "gateway/player_notifier.hpp"
#include "gateway/room_manager.hpp"

namespace gateway {

class TcpSession final : public std::enable_shared_from_this<TcpSession> {
public:
    using tcp = boost::asio::ip::tcp;

    TcpSession(tcp::socket socket,
               std::shared_ptr<RoomManager> room_manager,
               std::shared_ptr<BackendClient> backend_client,
               std::shared_ptr<PlayerNotifier> notifier,
               boost::asio::thread_pool& backend_pool);

    void start();

private:
    void read_header();
    void read_body(std::uint32_t packet_size);
    void handle_packet();
    void refresh_heartbeat();
    void send_packet(std::uint16_t message_id, const std::string& body);
    void write_next();
    void close();

    tcp::socket socket_;
    boost::asio::steady_timer heartbeat_timer_;
    std::array<std::uint8_t, 6> header_buffer_{};
    std::vector<std::uint8_t> packet_;
    std::deque<std::vector<std::uint8_t>> write_queue_;
    bool read_in_progress_ = false;
    std::uint16_t message_id_ = 0;
    std::string player_id_;
    bool closed_ = false;
    std::shared_ptr<RoomManager> room_manager_;
    std::shared_ptr<BackendClient> backend_client_;
    std::shared_ptr<PlayerNotifier> notifier_;
    boost::asio::thread_pool& backend_pool_;
};

class TcpServer final {
public:
    using tcp = boost::asio::ip::tcp;

    TcpServer(boost::asio::io_context& io_context,
              std::uint16_t port,
              std::shared_ptr<RoomManager> room_manager,
              std::shared_ptr<BackendClient> backend_client,
              std::shared_ptr<PlayerNotifier> notifier,
              boost::asio::thread_pool& backend_pool);

    void start();

private:
    void accept_next();
    void schedule_cleanup();

    tcp::acceptor acceptor_;
    boost::asio::steady_timer cleanup_timer_;
    std::shared_ptr<RoomManager> room_manager_;
    std::shared_ptr<BackendClient> backend_client_;
    std::shared_ptr<PlayerNotifier> notifier_;
    boost::asio::thread_pool& backend_pool_;
};

}  // namespace gateway
