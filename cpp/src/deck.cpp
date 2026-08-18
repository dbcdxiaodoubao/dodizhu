#include "game/deck.hpp"

#include <algorithm>

namespace game {
namespace {

std::vector<Card> create_cards() {
    std::vector<Card> cards;
    cards.reserve(54);

    for (auto rank = static_cast<std::uint8_t>(Rank::Three);
         rank <= static_cast<std::uint8_t>(Rank::Two);
         ++rank) {
        for (std::uint8_t suit = 0; suit < 4; ++suit) {
            cards.push_back(Card{static_cast<Rank>(rank), suit});
        }
    }

    cards.push_back(Card{Rank::SmallJoker, 0});
    cards.push_back(Card{Rank::BigJoker, 0});
    return cards;
}

}  // namespace

DealResult Deck::deal(std::mt19937& random) const {
    auto cards = create_cards();
    std::shuffle(cards.begin(), cards.end(), random);

    DealResult result;
    for (std::size_t index = 0; index < 51; ++index) {
        result.hands[index % 3].push_back(cards[index]);
    }
    for (std::size_t index = 0; index < result.bottom_cards.size(); ++index) {
        result.bottom_cards[index] = cards[51 + index];
    }

    return result;
}

}  // namespace game
