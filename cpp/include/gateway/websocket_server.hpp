#pragma once

#include <memory>

#include <boost/asio.hpp>

#include "gateway/backend_client.hpp"
#include "gateway/player_notifier.hpp"
#include "gateway/room_manager.hpp"

namespace gateway {

class WebSocketServer final {
public:
    using tcp = boost::asio::ip::tcp;

    WebSocketServer(boost::asio::io_context& io_context,
                    std::uint16_t port,
                    std::shared_ptr<RoomManager> room_manager,
                    std::shared_ptr<BackendClient> backend_client,
                    std::shared_ptr<PlayerNotifier> notifier,
                    boost::asio::thread_pool& backend_pool);

    void start();

private:
    void accept_next();

    tcp::acceptor acceptor_;
    std::shared_ptr<RoomManager> room_manager_;
    std::shared_ptr<BackendClient> backend_client_;
    std::shared_ptr<PlayerNotifier> notifier_;
    boost::asio::thread_pool& backend_pool_;
};

}  // namespace gateway
