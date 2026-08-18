#include "gateway/tcp_server.hpp"

#include <algorithm>
#include <chrono>
#include <iostream>

#include "gateway.pb.h"
#include "gateway/packet_codec.hpp"
#include "game/card_codec.hpp"

namespace gateway {
namespace {

constexpr std::uint32_t max_packet_size = 64U * 1024U;

}  // namespace

TcpSession::TcpSession(tcp::socket socket,
                       std::shared_ptr<RoomManager> room_manager,
                       std::shared_ptr<BackendClient> backend_client)
    : socket_(std::move(socket)),
      heartbeat_timer_(socket_.get_executor()),
      room_manager_(std::move(room_manager)),
      backend_client_(std::move(backend_client)) {
}

void TcpSession::start() {
    refresh_heartbeat();
    read_header();
}

void TcpSession::read_header() {
    auto self = shared_from_this();
    boost::asio::async_read(
        socket_,
        boost::asio::buffer(header_buffer_),
        [self](const boost::system::error_code& error, std::size_t) {
            if (error) {
                self->close();
                return;
            }

            const auto header = PacketCodec::decode_header(self->header_buffer_);
            if (!header || header->packet_size > max_packet_size) {
                self->close();
                return;
            }

            self->message_id_ = header->message_id;
            self->read_body(header->packet_size);
        });
}

void TcpSession::read_body(std::uint32_t packet_size) {
    const auto body_size = packet_size - PacketCodec::header_size;
    packet_.resize(packet_size);
    std::copy(header_buffer_.begin(), header_buffer_.end(), packet_.begin());

    if (body_size == 0) {
        write_packet();
        return;
    }

    auto self = shared_from_this();
    boost::asio::async_read(
        socket_,
        boost::asio::buffer(packet_.data() + PacketCodec::header_size, body_size),
        [self](const boost::system::error_code& error, std::size_t) {
            if (error) {
                self->close();
                return;
            }

            self->handle_packet();
        });
}

void TcpSession::handle_packet() {
    refresh_heartbeat();
    const auto* body = packet_.data() + PacketCodec::header_size;
    const auto body_size = packet_.size() - PacketCodec::header_size;

    switch (message_id_) {
    case doudizhu::C2S_LOGIN: {
        doudizhu::C2SLogin login;
        if (!login.ParseFromArray(body, static_cast<int>(body_size))) {
            close();
            return;
        }
        std::cout << "login player_id=" << login.player_id() << std::endl;
        const auto backend_login = backend_client_->login(login.player_id());
        doudizhu::S2CLoginRet login_ret;
        login_ret.set_success(backend_login.success);
        login_ret.set_player_id(backend_login.success ? backend_login.player_id : login.player_id());
        login_ret.set_coins(backend_login.coins);
        login_ret.set_message(backend_login.success ? "login accepted" : backend_login.message);
        if (backend_login.success) {
            player_id_ = backend_login.player_id;
        }
        send_packet(doudizhu::S2C_LOGIN_RET, login_ret.SerializeAsString());
        break;
    }
    case doudizhu::C2S_HEARTBEAT: {
        doudizhu::C2SHeartBeat heartbeat;
        if (!heartbeat.ParseFromArray(body, static_cast<int>(body_size))) {
            close();
            return;
        }
        doudizhu::S2CHeartBeatRet heartbeat_ret;
        const auto now = std::chrono::system_clock::now().time_since_epoch();
        heartbeat_ret.set_server_time(
            std::chrono::duration_cast<std::chrono::milliseconds>(now).count());
        send_packet(doudizhu::S2C_HEARTBEAT_RET, heartbeat_ret.SerializeAsString());
        break;
    }
    case doudizhu::C2S_QUICK_MATCH: {
        doudizhu::C2SQuickMatch quick_match;
        if (!quick_match.ParseFromArray(body, static_cast<int>(body_size))) {
            close();
            return;
        }

        doudizhu::S2CMatchRet match_ret;
        if (player_id_.empty()) {
            match_ret.set_success(false);
            match_ret.set_message("login required");
        } else {
            const auto result = room_manager_->match(player_id_);
            match_ret.set_success(true);
            match_ret.set_room_id(result.room_id);
            match_ret.set_seat(result.seat);
            match_ret.set_game_started(result.game_started);
            match_ret.set_message(result.game_started ? "game ready" : "waiting for players");
        }
        send_packet(doudizhu::S2C_MATCH_RET, match_ret.SerializeAsString());
        break;
    }
    case doudizhu::C2S_CALL_LANDLORD: {
        doudizhu::C2SCallLandlord call_request;
        if (!call_request.ParseFromArray(body, static_cast<int>(body_size))) {
            close();
            return;
        }

        const auto result = room_manager_->call_landlord(player_id_, call_request.call());
        if (!result) {
            close();
            return;
        }

        doudizhu::S2CCallLandlordRet call_ret;
        call_ret.set_accepted(result->accepted);
        call_ret.set_current_seat(result->current_seat);
        call_ret.set_game_started(result->game_started);
        if (result->landlord_seat) {
            call_ret.set_landlord_seat(*result->landlord_seat);
        }
        send_packet(doudizhu::S2C_CALL_LANDLORD_RET, call_ret.SerializeAsString());
        break;
    }
    case doudizhu::C2S_PLAY_CARDS: {
        doudizhu::C2SPlayCards play_request;
        if (!play_request.ParseFromArray(body, static_cast<int>(body_size))) {
            close();
            return;
        }

        std::vector<game::Card> cards;
        cards.reserve(play_request.cards_size());
        for (const auto encoded : play_request.cards()) {
            const auto card = game::CardCodec::decode(encoded);
            if (!card) {
                close();
                return;
            }
            cards.push_back(*card);
        }

        const auto result = room_manager_->play_cards(player_id_, cards);
        if (!result) {
            close();
            return;
        }

        doudizhu::S2CPlayCardsRet play_ret;
        play_ret.set_accepted(result->accepted);
        play_ret.set_current_seat(result->current_seat);
        play_ret.set_game_over(result->game_over);
        send_packet(doudizhu::S2C_PLAY_CARDS_RET, play_ret.SerializeAsString());
        break;
    }
    case doudizhu::C2S_PASS: {
        doudizhu::C2SPass pass_request;
        if (!pass_request.ParseFromArray(body, static_cast<int>(body_size))) {
            close();
            return;
        }

        const auto result = room_manager_->pass(player_id_);
        if (!result) {
            close();
            return;
        }

        doudizhu::S2CPassRet pass_ret;
        pass_ret.set_accepted(result->accepted);
        pass_ret.set_current_seat(result->current_seat);
        pass_ret.set_game_over(result->game_over);
        send_packet(doudizhu::S2C_PASS_RET, pass_ret.SerializeAsString());
        break;
    }
    default:
        std::cout << "unknown message_id=" << message_id_ << std::endl;
        close();
        break;
    }
}

void TcpSession::refresh_heartbeat() {
    heartbeat_timer_.expires_after(std::chrono::seconds(5));
    auto self = shared_from_this();
    heartbeat_timer_.async_wait([self](const boost::system::error_code& error) {
        if (!error) {
            self->close();
        }
    });
}

void TcpSession::send_packet(std::uint16_t message_id, const std::string& body) {
    const auto packet_size = PacketCodec::header_size + body.size();
    if (packet_size > max_packet_size) {
        close();
        return;
    }

    const auto header = PacketCodec::encode_header(
        PacketHeader{static_cast<std::uint32_t>(packet_size), message_id});
    packet_.resize(packet_size);
    std::copy(header.begin(), header.end(), packet_.begin());
    std::copy(body.begin(), body.end(), packet_.begin() + PacketCodec::header_size);
    write_packet();
}

void TcpSession::write_packet() {
    auto self = shared_from_this();
    boost::asio::async_write(
        socket_,
        boost::asio::buffer(packet_),
        [self](const boost::system::error_code& error, std::size_t) {
            if (error) {
                self->close();
                return;
            }

            self->read_header();
        });
}

void TcpSession::close() {
    boost::system::error_code ignored;
    heartbeat_timer_.cancel();
    socket_.shutdown(tcp::socket::shutdown_both, ignored);
    socket_.close(ignored);
}

TcpServer::TcpServer(boost::asio::io_context& io_context, std::uint16_t port)
    : acceptor_(io_context, tcp::endpoint(tcp::v4(), port)) {
}

void TcpServer::start() {
    accept_next();
}

void TcpServer::accept_next() {
    acceptor_.async_accept(
        [this](const boost::system::error_code& error, tcp::socket socket) {
            if (!error) {
                std::make_shared<TcpSession>(std::move(socket), room_manager_, backend_client_)->start();
            }

            accept_next();
        });
}

}  // namespace gateway
