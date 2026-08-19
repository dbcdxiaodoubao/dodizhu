#include <chrono>
#include <cassert>

#include "gateway/room_manager.hpp"

int main() {
    gateway::RoomManager rooms;

    const auto first = rooms.match("player-a");
    const auto second = rooms.match("player-b");
    const auto third = rooms.match("player-c");
    const auto fourth = rooms.match("player-d");

    assert(first.room_id == 1);
    assert(first.seat == 0);
    assert(!first.game_started);
    assert(second.room_id == 1);
    assert(second.seat == 1);
    assert(!second.game_started);
    assert(third.room_id == 1);
    assert(third.seat == 2);
    assert(third.game_started);
    assert(fourth.room_id == 2);
    assert(fourth.seat == 0);
    assert(!fourth.game_started);

    const auto call_first = rooms.call_landlord("player-a", true);
    const auto call_second = rooms.call_landlord("player-b", false);
    const auto call_third = rooms.call_landlord("player-c", false);

    assert(call_first.has_value());
    assert(call_first->accepted);
    assert(call_first->current_seat == 1);
    assert(call_third.has_value());
    assert(call_third->game_started);
    assert(call_third->landlord_seat == 0);

    const auto now = std::chrono::steady_clock::time_point{};
    rooms.mark_offline("player-a", now);
    const auto expired = rooms.cleanup_expired(now + std::chrono::minutes(6));
    assert(expired.size() == 1);
    assert(expired[0].settlements.size() == 3);

    gateway::RoomManager timeout_rooms;
    timeout_rooms.match("timeout-a");
    timeout_rooms.match("timeout-b");
    timeout_rooms.match("timeout-c");
    const auto timeout_start = std::chrono::steady_clock::now();
    timeout_rooms.timeout_expired(timeout_start + std::chrono::seconds(11));
    const auto timeout_info = timeout_rooms.reconnect_info("timeout-a");
    assert(timeout_info.has_value());
    assert(timeout_info->current_seat == 1);

    gateway::RoomManager idle_rooms;
    const auto idle_match = idle_rooms.match("idle-player");
    idle_rooms.cleanup_idle(std::chrono::steady_clock::now() + std::chrono::seconds(61));
    const auto next_match = idle_rooms.match("next-player");
    assert(next_match.room_id == idle_match.room_id + 1);

    const auto state = timeout_rooms.room_state("timeout-a");
    assert(state.has_value());
    assert(state->players[0].player_id == "timeout-a");
    assert(state->own_hand.size() == 17);
    assert(state->players[0].remaining_cards == 17);

    gateway::RoomManager finished_rooms;
    finished_rooms.match("finish-a");
    finished_rooms.match("finish-b");
    finished_rooms.match("finish-c");
    finished_rooms.call_landlord("finish-a", true);
    finished_rooms.call_landlord("finish-b", false);
    finished_rooms.call_landlord("finish-c", false);
    auto finished_now = std::chrono::steady_clock::now();
    for (int step = 0; step < 80; ++step) {
        finished_now += std::chrono::seconds(20);
        finished_rooms.timeout_expired(finished_now);
    }
    const auto next_game = finished_rooms.match("finish-a");
    assert(next_game.room_id == 2);
}
