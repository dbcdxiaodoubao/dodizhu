#pragma once

#include <cstdint>
#include <vector>

#include "game/deck.hpp"

namespace game {

enum class CardPattern : std::uint8_t {
    Invalid,
    Single,
    Pair,
    Triple,
    TripleWithSingle,
    TripleWithPair,
    Straight,
    ConsecutivePairs,
    Airplane,
    AirplaneWithSingles,
    AirplaneWithPairs,
    Bomb,
    Rocket,
};

struct Play {
    CardPattern pattern = CardPattern::Invalid;
    Rank main_rank = Rank::Three;
    std::size_t card_count = 0;
};

class CardRules final {
public:
    static Play analyze(const std::vector<Card>& cards);
};

}  // namespace game
