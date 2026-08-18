#pragma once

#include <cstdint>
#include <optional>

#include "game/deck.hpp"

namespace game {

class CardCodec final {
public:
    static std::uint32_t encode(const Card& card);
    static std::optional<Card> decode(std::uint32_t encoded);
};

}  // namespace game
