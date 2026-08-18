#pragma once

#include <array>
#include <cstdint>
#include <optional>

namespace gateway {

struct PacketHeader {
    std::uint32_t packet_size;
    std::uint16_t message_id;
};

class PacketCodec {
public:
    static constexpr std::size_t header_size = 6;

    static std::optional<PacketHeader> decode_header(
        const std::array<std::uint8_t, header_size>& bytes);
};

}  // namespace gateway
