#include <array>
#include <cassert>

#include "gateway/packet_codec.hpp"

int main() {
    const auto header = gateway::PacketCodec::decode_header({0, 0, 0, 9, 0, 42});

    assert(header.has_value());
    assert(header->packet_size == 9);
    assert(header->message_id == 42);
}
