#pragma once

#include <array>
#include <chrono>
#include <cstdint>
#include <optional>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "game/game_round.hpp"

namespace gateway {

struct MatchResult {
    std::uint64_t room_id;
    std::uint8_t seat;
    bool game_started;
    struct GameStartInfo {
        std::array<std::string, 3> player_ids;
        std::array<std::vector<game::Card>, 3> hands;
        std::uint8_t current_seat;
    };
    std::optional<GameStartInfo> game_start;
};

struct CallLandlordResult {
    bool accepted;
    std::uint8_t current_seat;
    bool game_started;
    std::optional<std::uint8_t> landlord_seat;
    std::uint8_t actor_seat;
    bool called;
    std::array<std::string, 3> player_ids;
};

struct PlayCardsResult {
    bool accepted;
    std::uint8_t current_seat;
    bool game_over;
    struct SettlementEntry {
        std::string game_id;
        std::string player_id;
        int coin_change;
        std::string result;
        long long duration_seconds;
    };
    std::vector<SettlementEntry> settlements;
    std::uint8_t actor_seat;
    std::array<std::string, 3> player_ids;
};

struct ReconnectInfo {
    std::uint64_t room_id;
    std::uint8_t seat;
    game::GamePhase phase;
    std::uint8_t current_seat;
    std::optional<std::uint8_t> landlord_seat;
    std::vector<game::Card> hand;
};

struct ExpiredRoom {
    std::vector<PlayCardsResult::SettlementEntry> settlements;
};

struct TimeoutAction {
    std::array<std::string, 3> player_ids;
    std::uint8_t actor_seat;
    game::GamePhase phase;
    std::uint8_t current_seat;
    std::vector<PlayCardsResult::SettlementEntry> settlements;
};

struct RoomState {
    struct PlayerInfo {
        std::string player_id;
        bool online;
        std::uint32_t remaining_cards;
    };
    std::uint64_t room_id;
    std::uint8_t self_seat;
    game::GamePhase phase;
    std::uint8_t current_seat;
    std::optional<std::uint8_t> landlord_seat;
    std::array<PlayerInfo, 3> players;
    std::vector<game::Card> own_hand;
    std::vector<game::Card> bottom_cards;
    std::optional<std::uint8_t> last_play_seat;
    std::vector<game::Card> last_play_cards;
};

class RoomManager final {
public:
    MatchResult match(const std::string& player_id);
    std::optional<CallLandlordResult> call_landlord(const std::string& player_id, bool call);
    std::optional<PlayCardsResult> play_cards(
        const std::string& player_id, const std::vector<game::Card>& cards);
    std::optional<PlayCardsResult> pass(const std::string& player_id);
    std::optional<ReconnectInfo> reconnect_info(const std::string& player_id) const;
    std::optional<RoomState> room_state(const std::string& player_id) const;
    void mark_online(const std::string& player_id);
    void mark_offline(const std::string& player_id, std::chrono::steady_clock::time_point now);
    std::vector<TimeoutAction> timeout_expired(std::chrono::steady_clock::time_point now);
    void cleanup_idle(std::chrono::steady_clock::time_point now);
    std::vector<ExpiredRoom> cleanup_expired(std::chrono::steady_clock::time_point now);

private:
    struct Room {
        std::uint64_t id;
        std::vector<std::string> player_ids;
        std::optional<game::GameRound> round;
        std::chrono::steady_clock::time_point action_deadline{};
        std::chrono::steady_clock::time_point last_activity = std::chrono::steady_clock::now();
    };

    std::uint64_t next_room_id_ = 1;
    std::optional<std::uint64_t> waiting_room_id_;
    std::unordered_map<std::uint64_t, Room> rooms_;
    std::unordered_map<std::string, std::uint64_t> player_rooms_;
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> offline_since_;
    std::mt19937 random_{std::random_device{}()};
    mutable std::mutex mutex_;

    void refresh_deadline(Room& room, std::chrono::steady_clock::time_point now);
};

}  // namespace gateway
