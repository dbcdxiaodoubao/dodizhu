#pragma once

#include <array>
#include <cstdint>
#include <random>
#include <vector>

namespace game {

enum class Rank : std::uint8_t {
    Three = 3,
    Four,
    Five,
    Six,
    Seven,
    Eight,
    Nine,
    Ten,
    Jack,
    Queen,
    King,
    Ace,
    Two,
    SmallJoker,
    BigJoker,
};

struct Card {
    Rank rank;
    std::uint8_t suit;

    bool operator==(const Card& other) const {
        return rank == other.rank && suit == other.suit;
    }
};

struct DealResult {
    std::array<std::vector<Card>, 3> hands;
    std::array<Card, 3> bottom_cards;
};

class Deck final {
public:
    DealResult deal(std::mt19937& random) const;
};

}  // namespace game
