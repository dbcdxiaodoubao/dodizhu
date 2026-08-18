cmake_minimum_required(VERSION 3.25)
project(doudizhu_gateway LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

cmake_policy(SET CMP0167 NEW)
find_package(Boost REQUIRED COMPONENTS asio system)

add_executable(gateway
    src/main.cpp
    src/tcp_server.cpp
)
target_include_directories(gateway PRIVATE include)
target_link_libraries(gateway PRIVATE packet_codec Boost::asio Boost::system ws2_32 mswsock)

add_library(packet_codec src/packet_codec.cpp)
target_include_directories(packet_codec PUBLIC include)

enable_testing()
add_executable(packet_codec_test tests/packet_codec_test.cpp)
target_link_libraries(packet_codec_test PRIVATE packet_codec)
add_test(NAME packet_codec_test COMMAND packet_codec_test)
