#include "gateway/backend_client.hpp"

#include <utility>

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/http.hpp>
#include <boost/json.hpp>

namespace gateway {
namespace {

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
using tcp = asio::ip::tcp;

}  // namespace

BackendClient::BackendClient(std::string host, std::string port)
    : host_(std::move(host)), port_(std::move(port)) {
}

LoginResult BackendClient::login(const std::string& player_id) const {
    try {
        asio::io_context io_context;
        tcp::resolver resolver(io_context);
        beast::tcp_stream stream(io_context);
        stream.connect(resolver.resolve(host_, port_));

        boost::json::object request_body;
        request_body["playerId"] = player_id;

        http::request<http::string_body> request{http::verb::post, "/api/auth/login", 11};
        request.set(http::field::host, host_);
        request.set(http::field::content_type, "application/json");
        request.body() = boost::json::serialize(request_body);
        request.prepare_payload();
        http::write(stream, request);

        beast::flat_buffer buffer;
        http::response<http::string_body> response;
        http::read(stream, buffer, response);

        boost::system::error_code ignored;
        stream.socket().shutdown(tcp::socket::shutdown_both, ignored);

        if (response.result() != http::status::ok) {
            return LoginResult{false, {}, 0, "backend login failed"};
        }
        return parse_login_response(response.body());
    } catch (const std::exception& error) {
        return LoginResult{false, {}, 0, error.what()};
    }
}

LoginResult BackendClient::settle(const std::string& game_id,
                                  const std::string& player_id,
                                  int coin_change,
                                  const std::string& result,
                                  long long duration_seconds) const {
    try {
        asio::io_context io_context;
        tcp::resolver resolver(io_context);
        beast::tcp_stream stream(io_context);
        stream.connect(resolver.resolve(host_, port_));

        boost::json::object request_body;
        request_body["gameId"] = game_id;
        request_body["playerId"] = player_id;
        request_body["coinChange"] = coin_change;
        request_body["result"] = result;
        request_body["durationSeconds"] = duration_seconds;

        http::request<http::string_body> request{http::verb::post, "/api/games/settle", 11};
        request.set(http::field::host, host_);
        request.set(http::field::content_type, "application/json");
        request.body() = boost::json::serialize(request_body);
        request.prepare_payload();
        http::write(stream, request);

        beast::flat_buffer buffer;
        http::response<http::string_body> response;
        http::read(stream, buffer, response);

        boost::system::error_code ignored;
        stream.socket().shutdown(tcp::socket::shutdown_both, ignored);

        if (response.result() != http::status::ok) {
            return LoginResult{false, {}, 0, "backend settlement failed"};
        }
        return parse_login_response(response.body());
    } catch (const std::exception& error) {
        return LoginResult{false, {}, 0, error.what()};
    }
}

void BackendClient::mark_offline(const std::string& player_id) const {
    try {
        asio::io_context io_context;
        tcp::resolver resolver(io_context);
        beast::tcp_stream stream(io_context);
        stream.connect(resolver.resolve(host_, port_));

        http::request<http::empty_body> request{
            http::verb::post, "/api/players/" + player_id + "/offline", 11};
        request.set(http::field::host, host_);
        http::write(stream, request);

        beast::flat_buffer buffer;
        http::response<http::empty_body> response;
        http::read(stream, buffer, response);

        boost::system::error_code ignored;
        stream.socket().shutdown(tcp::socket::shutdown_both, ignored);
    } catch (const std::exception&) {
    }
}

LoginResult BackendClient::parse_login_response(const std::string& body) {
    try {
        const auto value = boost::json::parse(body);
        const auto& object = value.as_object();
        const auto player = object.if_contains("playerId");
        const auto coins = object.if_contains("coins");
        if (!player || !coins || !player->is_string() || !coins->is_int64()) {
            return LoginResult{false, {}, 0, "invalid backend response"};
        }

        return LoginResult{
            true,
            std::string(player->as_string()),
            static_cast<int>(coins->as_int64()),
            {},
        };
    } catch (const std::exception& error) {
        return LoginResult{false, {}, 0, error.what()};
    }
}

}  // namespace gateway
