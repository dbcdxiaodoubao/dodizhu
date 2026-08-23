#pragma once

#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>

namespace gateway {

class PlayerNotifier final {
public:
    using SendFunction = std::function<void(std::uint16_t, const std::string&)>;

    std::uint64_t register_player(const std::string& player_id, SendFunction send) {
        std::lock_guard lock(mutex_);
        const auto token = next_token_++;
        senders_[player_id] = Entry{token, std::move(send)};
        return token;
    }

    void unregister_player(const std::string& player_id, std::uint64_t token) {
        std::lock_guard lock(mutex_);
        const auto found = senders_.find(player_id);
        if (found != senders_.end() && found->second.token == token) {
            senders_.erase(found);
        }
    }

    void send(const std::string& player_id, std::uint16_t message_id, const std::string& body) const {
        SendFunction sender;
        {
            std::lock_guard lock(mutex_);
            const auto found = senders_.find(player_id);
            if (found == senders_.end()) {
                return;
            }
            sender = found->second.send;
        }
        sender(message_id, body);
    }

private:
    struct Entry {
        std::uint64_t token;
        SendFunction send;
    };

    std::unordered_map<std::string, Entry> senders_;
    std::uint64_t next_token_ = 1;
    mutable std::mutex mutex_;
};

}  // namespace gateway
