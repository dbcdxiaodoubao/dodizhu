#include <cassert>
#include <cstdint>
#include <string>

#include "gateway.pb.h"
#include "gateway/packet_codec.hpp"

int main() {
    doudizhu::C2SLogin login;
    login.set_player_id("protocol-player");
    const auto body = login.SerializeAsString();

    const auto header = gateway::PacketCodec::encode_header({
        static_cast<std::uint32_t>(gateway::PacketCodec::header_size + body.size()),
        doudizhu::C2S_LOGIN,
    });
    const auto decoded_header = gateway::PacketCodec::decode_header(header);
    assert(decoded_header.has_value());
    assert(decoded_header->message_id == doudizhu::C2S_LOGIN);
    assert(decoded_header->packet_size == gateway::PacketCodec::header_size + body.size());

    doudizhu::C2SLogin decoded_login;
    assert(decoded_login.ParseFromString(body));
    assert(decoded_login.player_id() == "protocol-player");
}
