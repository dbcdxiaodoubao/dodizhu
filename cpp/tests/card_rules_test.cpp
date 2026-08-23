#include <cassert>
#include <vector>

#include "game/card_rules.hpp"

int main() {
    using game::Card;
    using game::CardPattern;
    using game::Rank;

    assert(game::CardRules::analyze({Card{Rank::Three, 0}}).pattern == CardPattern::Single);
    assert(game::CardRules::analyze({Card{Rank::Four, 0}, Card{Rank::Four, 1}}).pattern == CardPattern::Pair);
    assert(game::CardRules::analyze({Card{Rank::Five, 0}, Card{Rank::Five, 1}, Card{Rank::Five, 2}}).pattern == CardPattern::Triple);
    assert(game::CardRules::analyze({Card{Rank::Six, 0}, Card{Rank::Six, 1}, Card{Rank::Six, 2}, Card{Rank::Seven, 0}}).pattern == CardPattern::TripleWithSingle);
    assert(game::CardRules::analyze({Card{Rank::Three, 0}, Card{Rank::Four, 0}, Card{Rank::Five, 0}, Card{Rank::Six, 0}, Card{Rank::Seven, 0}}).pattern == CardPattern::Straight);
    assert(game::CardRules::analyze({
        Card{Rank::Three, 0}, Card{Rank::Three, 1}, Card{Rank::Three, 2},
        Card{Rank::Four, 0}, Card{Rank::Four, 1}, Card{Rank::Four, 2},
        Card{Rank::Five, 0}, Card{Rank::Six, 0}
    }).pattern == CardPattern::AirplaneWithSingles);
    assert(game::CardRules::analyze({
        Card{Rank::Three, 0}, Card{Rank::Three, 1}, Card{Rank::Three, 2},
        Card{Rank::Four, 0}, Card{Rank::Four, 1}, Card{Rank::Four, 2},
        Card{Rank::Five, 0}, Card{Rank::Five, 1},
        Card{Rank::Six, 0}, Card{Rank::Six, 1}
    }).pattern == CardPattern::AirplaneWithPairs);
    assert(game::CardRules::analyze({
        Card{Rank::Seven, 0}, Card{Rank::Seven, 1}, Card{Rank::Seven, 2}, Card{Rank::Seven, 3},
        Card{Rank::Three, 0}, Card{Rank::Four, 0}
    }).pattern == CardPattern::FourWithTwoSingles);
    assert(game::CardRules::analyze({
        Card{Rank::Eight, 0}, Card{Rank::Eight, 1}, Card{Rank::Eight, 2}, Card{Rank::Eight, 3},
        Card{Rank::Three, 0}, Card{Rank::Three, 1}, Card{Rank::Four, 0}, Card{Rank::Four, 1}
    }).pattern == CardPattern::FourWithTwoPairs);
    assert(game::CardRules::analyze({Card{Rank::Eight, 0}, Card{Rank::Eight, 1}, Card{Rank::Eight, 2}, Card{Rank::Eight, 3}}).pattern == CardPattern::Bomb);
    assert(game::CardRules::analyze({Card{Rank::SmallJoker, 0}, Card{Rank::BigJoker, 0}}).pattern == CardPattern::Rocket);

    const auto pair_four = game::CardRules::analyze({Card{Rank::Four, 0}, Card{Rank::Four, 1}});
    const auto pair_five = game::CardRules::analyze({Card{Rank::Five, 0}, Card{Rank::Five, 1}});
    const auto bomb = game::CardRules::analyze({Card{Rank::Six, 0}, Card{Rank::Six, 1}, Card{Rank::Six, 2}, Card{Rank::Six, 3}});
    const auto rocket = game::CardRules::analyze({Card{Rank::SmallJoker, 0}, Card{Rank::BigJoker, 0}});
    assert(game::CardRules::can_beat(pair_five, pair_four));
    assert(!game::CardRules::can_beat(pair_four, pair_five));
    assert(game::CardRules::can_beat(bomb, pair_five));
    assert(game::CardRules::can_beat(rocket, bomb));
}
