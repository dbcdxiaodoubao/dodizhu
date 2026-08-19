#include <cstdint>
#include <iostream>
#include <cstdlib>

#include <boost/asio.hpp>

#include "gateway/tcp_server.hpp"
#include "gateway/websocket_server.hpp"

int main() {
    try {
        boost::asio::io_context io_context;
        auto room_manager = std::make_shared<gateway::RoomManager>();
        const auto backend_host = std::getenv("BACKEND_HOST");
        const auto backend_port = std::getenv("BACKEND_PORT");
        auto backend_client = std::make_shared<gateway::BackendClient>(
            backend_host ? backend_host : "127.0.0.1",
            backend_port ? backend_port : "8080");
        auto notifier = std::make_shared<gateway::PlayerNotifier>();
        boost::asio::thread_pool backend_pool(2);
        gateway::TcpServer tcp_server(io_context, 9000, room_manager, backend_client, notifier, backend_pool);
        gateway::WebSocketServer websocket_server(io_context, 9001, room_manager, backend_client, notifier, backend_pool);
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
