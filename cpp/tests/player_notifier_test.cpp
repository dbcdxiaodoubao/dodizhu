#include <cassert>
#include <string>

#include "gateway/player_notifier.hpp"

int main() {
    gateway::PlayerNotifier notifier;
    std::uint16_t received_message_id = 0;
    std::string received_body;
    const auto token = notifier.register_player("alice", [&](std::uint16_t message_id, const std::string& body) {
        received_message_id = message_id;
        received_body = body;
    });

    notifier.send("alice", 13, "payload");
    assert(received_message_id == 13);
    assert(received_body == "payload");

    notifier.unregister_player("alice", token);
    notifier.send("alice", 14, "ignored");
    assert(received_message_id == 13);

    const auto old_token = notifier.register_player("bob", [](std::uint16_t, const std::string&) {});
    std::uint16_t new_connection_message = 0;
    notifier.register_player("bob", [&](std::uint16_t message_id, const std::string&) {
        new_connection_message = message_id;
    });
    notifier.unregister_player("bob", old_token);
    notifier.send("bob", 15, "still connected");
    assert(new_connection_message == 15);
}
