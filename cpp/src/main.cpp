#include <cstdint>
#include <iostream>

#include <boost/asio.hpp>

#include "gateway/tcp_server.hpp"

int main() {
    try {
        boost::asio::io_context io_context;
        gateway::TcpServer server(io_context, 9000);
        server.start();

        std::cout << "TCP gateway listening on port 9000" << std::endl;
        io_context.run();
    } catch (const std::exception& error) {
        std::cerr << "Gateway stopped: " << error.what() << std::endl;
        return 1;
    }

    return 0;
}
