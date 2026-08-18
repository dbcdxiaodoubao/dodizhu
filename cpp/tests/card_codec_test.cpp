#include <cassert>

#include "game/card_codec.hpp"

int main() {
    const game::Card card{game::Rank::Ace, 2};
    const auto encoded = game::CardCodec::encode(card);
    const auto decoded = game::CardCodec::decode(encoded);

    assert(decoded.has_value());
    assert(*decoded == card);
    assert(!game::CardCodec::decode(1).has_value());
}
