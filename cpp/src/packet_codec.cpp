#include "gateway/packet_codec.hpp"

namespace gateway {

std::optional<PacketHeader> PacketCodec::decode_header(
    const std::array<std::uint8_t, header_size>& bytes) {
    // The wire format uses big-endian integers, independent of the CPU byte order.
    const auto packet_size = (static_cast<std::uint32_t>(bytes[0]) << 24U) |
                             (static_cast<std::uint32_t>(bytes[1]) << 16U) |
                             (static_cast<std::uint32_t>(bytes[2]) << 8U) |
                             static_cast<std::uint32_t>(bytes[3]);
    const auto message_id = static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(bytes[4]) << 8U) | bytes[5]);

    if (packet_size < header_size) {
        return std::nullopt;
    }

    return PacketHeader{packet_size, message_id};
}

}  // namespace gateway
