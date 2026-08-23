#pragma once

#include <string>

#include "gateway.pb.h"
#include "gateway/room_manager.hpp"
#include "game/card_codec.hpp"

namespace gateway {

inline std::string encode_room_state(const RoomState& state) {
    doudizhu::S2CRoomState message;
    message.set_room_id(state.room_id);
    message.set_self_seat(state.self_seat);
    message.set_phase(static_cast<std::uint32_t>(state.phase));
    message.set_current_seat(state.current_seat);
    if (state.landlord_seat) {
        message.set_landlord_seat(*state.landlord_seat);
    }
    for (std::uint32_t seat = 0; seat < state.players.size(); ++seat) {
        auto* player = message.add_players();
        player->set_seat(seat);
        player->set_player_id(state.players[seat].player_id);
        player->set_online(state.players[seat].online);
        player->set_remaining_cards(state.players[seat].remaining_cards);
    }
    for (const auto& card : state.own_hand) {
        message.add_own_cards(game::CardCodec::encode(card));
    }
    for (const auto& card : state.bottom_cards) {
        message.add_bottom_cards(game::CardCodec::encode(card));
    }
    if (state.last_play_seat) {
        message.set_last_play_seat(*state.last_play_seat);
    }
    for (const auto& card : state.last_play_cards) {
        message.add_last_play_cards(game::CardCodec::encode(card));
    }
    message.set_action_remaining_seconds(state.action_remaining_seconds);
    return message.SerializeAsString();
}

}  // namespace gateway
