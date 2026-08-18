#include <cassert>
#include <cstdint>
#include <random>
#include <set>

#include "game/deck.hpp"

int main() {
    game::Deck deck;
    std::mt19937 random(42);
    const auto deal = deck.deal(random);

    assert(deal.hands[0].size() == 17);
    assert(deal.hands[1].size() == 17);
    assert(deal.hands[2].size() == 17);
    assert(deal.bottom_cards.size() == 3);

    std::set<std::uint16_t> unique_cards;
    for (const auto& hand : deal.hands) {
        for (const auto& card : hand) {
            unique_cards.insert(static_cast<std::uint16_t>(card.rank) * 4U + card.suit);
        }
    }
    for (const auto& card : deal.bottom_cards) {
        unique_cards.insert(static_cast<std::uint16_t>(card.rank) * 4U + card.suit);
    }

    assert(unique_cards.size() == 54);
}
