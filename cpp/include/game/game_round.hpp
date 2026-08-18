#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <random>
#include <string>
#include <vector>

#include "game/deck.hpp"
#include "game/card_rules.hpp"

namespace game {

enum class GamePhase : std::uint8_t {
    WaitingPlayers,
    CallLandlord,
    PlayCards,
    Settled,
};

class GameRound final {
public:
    explicit GameRound(std::array<std::string, 3> player_ids);

    void start(std::mt19937& random);
    bool call_landlord(std::uint8_t seat, bool call, std::mt19937& random);
    bool play_cards(std::uint8_t seat, const std::vector<Card>& cards);
    bool pass(std::uint8_t seat);

    GamePhase phase() const;
    std::uint8_t current_seat() const;
    std::optional<std::uint8_t> landlord_seat() const;
    const std::vector<Card>& hand(std::uint8_t seat) const;

private:
    Deck deck_;
    std::array<std::string, 3> player_ids_;
    DealResult deal_{};
    GamePhase phase_ = GamePhase::WaitingPlayers;
    std::uint8_t current_seat_ = 0;
    std::uint8_t call_count_ = 0;
    std::optional<std::uint8_t> last_caller_;
    std::optional<std::uint8_t> landlord_seat_;
    std::optional<Play> last_play_;
    std::optional<std::uint8_t> last_play_seat_;
    std::uint8_t pass_count_ = 0;
};

}  // namespace game
