#include <cstdint>
#include <iostream>

#include <boost/asio.hpp>

#include "gateway/tcp_server.hpp"
#include "gateway/websocket_server.hpp"

int main() {
    try {
        boost::asio::io_context io_context;
        auto room_manager = std::make_shared<gateway::RoomManager>();
        auto backend_client = std::make_shared<gateway::BackendClient>();
        auto notifier = std::make_shared<gateway::PlayerNotifier>();
        gateway::TcpServer tcp_server(io_context, 9000, room_manager, backend_client, notifier);
        gateway::WebSocketServer websocket_server(io_context, 9001, room_manager, backend_client, notifier);
        tcp_server.start();
        websocket_server.start();

        std::cout << "TCP gateway listening on port 9000; WebSocket gateway listening on port 9001" << std::endl;
        io_context.run();
    } catch (const std::exception& error) {
        std::cerr << "Gateway stopped: " << error.what() << std::endl;
        return 1;
    }

    return 0;
}
