#pragma once

#include <string>

namespace gateway {

struct LoginResult {
    bool success = false;
    std::string player_id;
    int coins = 0;
    std::string message;
};

class BackendClient final {
public:
    BackendClient(std::string host = "127.0.0.1", std::string port = "8080");

    LoginResult login(const std::string& player_id) const;
    LoginResult settle(const std::string& game_id,
                       const std::string& player_id,
                       int coin_change,
                       const std::string& result,
                       long long duration_seconds) const;
    void mark_offline(const std::string& player_id) const;
    static LoginResult parse_login_response(const std::string& body);

private:
    std::string host_;
    std::string port_;
};

}  // namespace gateway
