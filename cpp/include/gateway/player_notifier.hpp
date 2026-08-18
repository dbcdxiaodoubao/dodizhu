#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>

namespace gateway {

class PlayerNotifier final {
public:
    using SendFunction = std::function<void(std::uint16_t, const std::string&)>;

    void register_player(const std::string& player_id, SendFunction send) {
        senders_[player_id] = std::move(send);
    }

    void send(const std::string& player_id, std::uint16_t message_id, const std::string& body) const {
        const auto sender = senders_.find(player_id);
        if (sender != senders_.end()) {
            sender->second(message_id, body);
        }
    }

private:
    std::unordered_map<std::string, SendFunction> senders_;
};

}  // namespace gateway
