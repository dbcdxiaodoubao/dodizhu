#include "game/card_codec.hpp"

namespace game {

std::uint32_t CardCodec::encode(const Card& card) {
    return static_cast<std::uint32_t>(card.rank) * 4U + card.suit;
}

std::optional<Card> CardCodec::decode(std::uint32_t encoded) {
    const auto rank_value = encoded / 4U;
    const auto suit = static_cast<std::uint8_t>(encoded % 4U);
    if (rank_value < static_cast<std::uint32_t>(Rank::Three) ||
        rank_value > static_cast<std::uint32_t>(Rank::BigJoker)) {
        return std::nullopt;
    }

    const auto rank = static_cast<Rank>(rank_value);
    if ((rank == Rank::SmallJoker || rank == Rank::BigJoker) && suit != 0) {
        return std::nullopt;
    }

    return Card{rank, suit};
}

}  // namespace game
