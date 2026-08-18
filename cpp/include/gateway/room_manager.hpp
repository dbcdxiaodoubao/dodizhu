#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "game/game_round.hpp"

namespace gateway {

struct MatchResult {
    std::uint64_t room_id;
    std::uint8_t seat;
    bool game_started;
};

struct CallLandlordResult {
    bool accepted;
    std::uint8_t current_seat;
    bool game_started;
    std::optional<std::uint8_t> landlord_seat;
};

struct PlayCardsResult {
    bool accepted;
    std::uint8_t current_seat;
    bool game_over;
};

class RoomManager final {
public:
    MatchResult match(const std::string& player_id);
    std::optional<CallLandlordResult> call_landlord(const std::string& player_id, bool call);
    std::optional<PlayCardsResult> play_cards(
        const std::string& player_id, const std::vector<game::Card>& cards);
    std::optional<PlayCardsResult> pass(const std::string& player_id);

private:
    struct Room {
        std::uint64_t id;
        std::vector<std::string> player_ids;
        std::optional<game::GameRound> round;
    };

    std::uint64_t next_room_id_ = 1;
    std::optional<std::uint64_t> waiting_room_id_;
    std::unordered_map<std::uint64_t, Room> rooms_;
    std::unordered_map<std::string, std::uint64_t> player_rooms_;
    std::mt19937 random_{std::random_device{}()};
};

}  // namespace gateway
