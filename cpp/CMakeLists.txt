cmake_minimum_required(VERSION 3.25)
project(doudizhu_gateway LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

cmake_policy(SET CMP0167 NEW)
find_package(Boost REQUIRED COMPONENTS asio system json)
find_package(Protobuf CONFIG REQUIRED)

set(PROTO_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/proto/gateway.proto)
set(PROTO_WORK_DIR D:/gateway-proto)
set(PROTO_GENERATED_DIR D:/gateway-generated)
set(PROTO_GENERATED_SOURCE ${PROTO_GENERATED_DIR}/gateway.pb.cc)
set(PROTO_GENERATED_HEADER ${PROTO_GENERATED_DIR}/gateway.pb.h)

add_custom_command(
    OUTPUT ${PROTO_GENERATED_SOURCE} ${PROTO_GENERATED_HEADER}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${PROTO_WORK_DIR}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${PROTO_GENERATED_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${PROTO_SOURCE} ${PROTO_WORK_DIR}/gateway.proto
    COMMAND ${Protobuf_PROTOC_EXECUTABLE} --cpp_out=${PROTO_GENERATED_DIR}
            -I ${PROTO_WORK_DIR} ${PROTO_WORK_DIR}/gateway.proto
    DEPENDS ${PROTO_SOURCE}
    VERBATIM
)

add_library(protocol ${PROTO_GENERATED_SOURCE})
target_include_directories(protocol PUBLIC ${PROTO_GENERATED_DIR})
target_link_libraries(protocol PUBLIC protobuf::libprotobuf)

add_executable(gateway
    src/main.cpp
    src/tcp_server.cpp
)
target_include_directories(gateway PRIVATE include)
target_link_libraries(gateway PRIVATE packet_codec card_codec protocol room_manager backend_client Boost::asio Boost::system Boost::json ws2_32 mswsock)
add_custom_command(TARGET gateway POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:protobuf::libprotobuf>
            $<TARGET_FILE_DIR:gateway>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:Boost::json>
            $<TARGET_FILE_DIR:gateway>
)

add_library(packet_codec src/packet_codec.cpp)
target_include_directories(packet_codec PUBLIC include)

add_library(room_manager src/room_manager.cpp)
target_include_directories(room_manager PUBLIC include)
target_link_libraries(room_manager PUBLIC game_round)

add_library(game_core src/deck.cpp)
target_include_directories(game_core PUBLIC include)

add_library(card_rules src/card_rules.cpp)
target_include_directories(card_rules PUBLIC include)
target_link_libraries(card_rules PUBLIC game_core)

add_library(card_codec src/card_codec.cpp)
target_include_directories(card_codec PUBLIC include)
target_link_libraries(card_codec PUBLIC game_core)

add_library(backend_client src/backend_client.cpp)
target_include_directories(backend_client PUBLIC include)
target_link_libraries(backend_client PUBLIC Boost::asio Boost::system Boost::json ws2_32)

add_library(game_round src/game_round.cpp)
target_include_directories(game_round PUBLIC include)
target_link_libraries(game_round PUBLIC card_rules game_core)

enable_testing()
add_executable(packet_codec_test tests/packet_codec_test.cpp)
target_link_libraries(packet_codec_test PRIVATE packet_codec)
add_test(NAME packet_codec_test COMMAND packet_codec_test)

add_executable(room_manager_test tests/room_manager_test.cpp)
target_link_libraries(room_manager_test PRIVATE room_manager)
add_test(NAME room_manager_test COMMAND room_manager_test)

add_executable(deck_test tests/deck_test.cpp)
target_link_libraries(deck_test PRIVATE game_core)
add_test(NAME deck_test COMMAND deck_test)

add_executable(card_rules_test tests/card_rules_test.cpp)
target_link_libraries(card_rules_test PRIVATE card_rules)
add_test(NAME card_rules_test COMMAND card_rules_test)

add_executable(card_codec_test tests/card_codec_test.cpp)
target_link_libraries(card_codec_test PRIVATE card_codec)
add_test(NAME card_codec_test COMMAND card_codec_test)

add_executable(backend_client_test tests/backend_client_test.cpp)
target_link_libraries(backend_client_test PRIVATE backend_client)
add_custom_command(TARGET backend_client_test POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:Boost::json>
            $<TARGET_FILE_DIR:backend_client_test>
)
add_test(NAME backend_client_test COMMAND backend_client_test)

add_executable(game_round_test tests/game_round_test.cpp)
target_link_libraries(game_round_test PRIVATE game_round)
add_test(NAME game_round_test COMMAND game_round_test)
