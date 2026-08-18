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
}
