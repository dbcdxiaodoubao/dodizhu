#include "gateway/room_manager.hpp"

#include <algorithm>

namespace gateway {

MatchResult RoomManager::match(const std::string& player_id) {
    for (const auto& [room_id, room] : rooms_) {
        const auto player = std::find(room.player_ids.begin(), room.player_ids.end(), player_id);
        if (player != room.player_ids.end()) {
            return MatchResult{
                room_id,
                static_cast<std::uint8_t>(std::distance(room.player_ids.begin(), player)),
                room.round.has_value(),
                std::nullopt,
            };
        }
    }

    if (!waiting_room_id_) {
        const auto room_id = next_room_id_++;
        rooms_.emplace(room_id, Room{room_id, {}});
        waiting_room_id_ = room_id;
    }

    auto& room = rooms_.at(*waiting_room_id_);
    room.player_ids.push_back(player_id);
    player_rooms_[player_id] = room.id;
    const auto game_started = room.player_ids.size() == 3;
    if (game_started) {
        room.round.emplace(std::array<std::string, 3>{
            room.player_ids[0], room.player_ids[1], room.player_ids[2]});
        room.round->start(random_);
    }
    auto result = MatchResult{
        room.id,
        static_cast<std::uint8_t>(room.player_ids.size() - 1),
        game_started,
        std::nullopt,
    };

    if (game_started) {
        waiting_room_id_.reset();
        result.game_start = MatchResult::GameStartInfo{
            {room.player_ids[0], room.player_ids[1], room.player_ids[2]},
            {room.round->hand(0), room.round->hand(1), room.round->hand(2)},
            room.round->current_seat(),
        };
    }

    return result;
}

std::optional<CallLandlordResult> RoomManager::call_landlord(
    const std::string& player_id, bool call) {
    const auto player_room = player_rooms_.find(player_id);
    if (player_room == player_rooms_.end()) {
        return std::nullopt;
    }

    auto& room = rooms_.at(player_room->second);
    if (!room.round) {
        return std::nullopt;
    }

    const auto player = std::find(room.player_ids.begin(), room.player_ids.end(), player_id);
    const auto seat = static_cast<std::uint8_t>(std::distance(room.player_ids.begin(), player));
    const auto accepted = room.round->call_landlord(seat, call, random_);
    return CallLandlordResult{
        accepted,
        room.round->current_seat(),
        room.round->phase() == game::GamePhase::PlayCards,
        room.round->landlord_seat(),
    };
}

std::optional<PlayCardsResult> RoomManager::play_cards(
    const std::string& player_id, const std::vector<game::Card>& cards) {
    const auto player_room = player_rooms_.find(player_id);
    if (player_room == player_rooms_.end()) {
        return std::nullopt;
    }

    auto& room = rooms_.at(player_room->second);
    if (!room.round) {
        return std::nullopt;
    }

    const auto player = std::find(room.player_ids.begin(), room.player_ids.end(), player_id);
    const auto seat = static_cast<std::uint8_t>(std::distance(room.player_ids.begin(), player));
    const auto accepted = room.round->play_cards(seat, cards);
    return PlayCardsResult{
        accepted,
        room.round->current_seat(),
        room.round->phase() == game::GamePhase::Settled,
    };
}

std::optional<PlayCardsResult> RoomManager::pass(const std::string& player_id) {
    const auto player_room = player_rooms_.find(player_id);
    if (player_room == player_rooms_.end()) {
        return std::nullopt;
    }

    auto& room = rooms_.at(player_room->second);
    if (!room.round) {
        return std::nullopt;
    }

    const auto player = std::find(room.player_ids.begin(), room.player_ids.end(), player_id);
    const auto seat = static_cast<std::uint8_t>(std::distance(room.player_ids.begin(), player));
    const auto accepted = room.round->pass(seat);
    return PlayCardsResult{
        accepted,
        room.round->current_seat(),
        room.round->phase() == game::GamePhase::Settled,
    };
}

}  // namespace gateway
