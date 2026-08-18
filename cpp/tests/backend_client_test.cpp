#include <cassert>

#include "gateway/backend_client.hpp"

int main() {
    const auto result = gateway::BackendClient::parse_login_response(
        "{\"playerId\":\"alice\",\"coins\":1000,\"created\":true}");

    assert(result.success);
    assert(result.player_id == "alice");
    assert(result.coins == 1000);
}
