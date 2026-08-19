#include <array>
#include <cassert>
#include <random>
#include <string>

#include "game/game_round.hpp"

int main() {
    game::GameRound round({"player-a", "player-b", "player-c"});
    std::mt19937 random(42);
    round.start(random);

    assert(round.phase() == game::GamePhase::CallLandlord);
    assert(round.current_seat() == 0);
    assert(round.hand(0).size() == 17);

    round.call_landlord(0, true, random);
    round.call_landlord(1, false, random);
    round.call_landlord(2, false, random);

    assert(round.phase() == game::GamePhase::PlayCards);
    assert(round.landlord_seat() == 0);
    assert(round.current_seat() == 0);
    assert(round.hand(0).size() == 20);

    const auto first_card = round.hand(0).front();
    assert(round.play_cards(0, {first_card}));
    assert(round.current_seat() == 1);
    assert(round.last_play_seat() == 0);
    assert(round.last_play_cards().size() == 1);
    assert(round.pass(1));
    assert(round.pass(2));
    assert(round.current_seat() == 0);
    assert(!round.last_play_seat().has_value());
    assert(round.last_play_cards().empty());

    assert(round.timeout(random));
    assert(round.hand(0).size() == 18);
    assert(round.current_seat() == 1);
}
