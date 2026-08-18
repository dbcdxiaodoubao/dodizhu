#include <cassert>
#include <string>

#include "gateway/player_notifier.hpp"

int main() {
    gateway::PlayerNotifier notifier;
    std::uint16_t received_message_id = 0;
    std::string received_body;
    notifier.register_player("alice", [&](std::uint16_t message_id, const std::string& body) {
        received_message_id = message_id;
        received_body = body;
    });

    notifier.send("alice", 13, "payload");
    assert(received_message_id == 13);
    assert(received_body == "payload");
}
