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
    PlayCardsResult result{
        accepted,
        room.round->current_seat(),
        room.round->phase() == game::GamePhase::Settled,
        {},
    };

    if (result.game_over && accepted) {
        const auto landlord = room.round->landlord_seat();
        const auto winner = room.round->winner_seat();
        if (landlord && winner) {
            const auto landlord_won = *landlord == *winner;
            for (std::uint8_t player_seat = 0; player_seat < room.player_ids.size(); ++player_seat) {
                const auto is_landlord = player_seat == *landlord;
                const auto won = is_landlord == landlord_won;
                result.settlements.push_back(PlayCardsResult::SettlementEntry{
                    room.player_ids[player_seat],
                    is_landlord ? (won ? 200 : -200) : (won ? 100 : -100),
                    won ? "WIN" : "LOSE",
                });
            }
        }
    }

    return result;
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
        {},
    };
}

std::optional<ReconnectInfo> RoomManager::reconnect_info(const std::string& player_id) const {
    const auto player_room = player_rooms_.find(player_id);
    if (player_room == player_rooms_.end()) {
        return std::nullopt;
    }

    const auto room = rooms_.find(player_room->second);
    if (room == rooms_.end() || !room->second.round) {
        return std::nullopt;
    }

    const auto player = std::find(room->second.player_ids.begin(), room->second.player_ids.end(), player_id);
    if (player == room->second.player_ids.end()) {
        return std::nullopt;
    }

    const auto seat = static_cast<std::uint8_t>(std::distance(room->second.player_ids.begin(), player));
    return ReconnectInfo{
        room->second.id,
        seat,
        room->second.round->phase(),
        room->second.round->current_seat(),
        room->second.round->landlord_seat(),
        room->second.round->hand(seat),
    };
}

}  // namespace gateway
