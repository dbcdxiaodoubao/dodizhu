#include "gateway/websocket_server.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <deque>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <boost/beast.hpp>
#include <boost/beast/websocket.hpp>

#include "gateway.pb.h"
#include "gateway/packet_codec.hpp"
#include "gateway/room_state_codec.hpp"
#include "game/card_codec.hpp"

namespace gateway {
namespace {

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace websocket = beast::websocket;
using tcp = asio::ip::tcp;

constexpr std::uint32_t max_packet_size = 64U * 1024U;

class WebSocketSession final : public std::enable_shared_from_this<WebSocketSession> {
public:
    WebSocketSession(tcp::socket socket,
                     std::shared_ptr<RoomManager> room_manager,
                     std::shared_ptr<BackendClient> backend_client,
                     std::shared_ptr<PlayerNotifier> notifier,
                     asio::thread_pool& backend_pool)
        : stream_(std::move(socket)),
          heartbeat_timer_(stream_.get_executor()),
          room_manager_(std::move(room_manager)),
          backend_client_(std::move(backend_client)),
          notifier_(std::move(notifier)),
          backend_pool_(backend_pool) {
    }

    void start() {
        stream_.binary(true);
        auto self = shared_from_this();
        stream_.async_accept([self](const boost::system::error_code& error) {
            if (error) {
                self->close();
                return;
            }
            self->refresh_heartbeat();
            self->read_next();
        });
    }

private:
    void read_next() {
        if (read_in_progress_) {
            return;
        }
        read_in_progress_ = true;
        auto self = shared_from_this();
        stream_.async_read(read_buffer_, [self](const boost::system::error_code& error, std::size_t) {
            if (error || !self->stream_.got_binary()) {
                self->close();
                return;
            }

            self->read_in_progress_ = false;
            self->packet_.resize(self->read_buffer_.size());
            asio::buffer_copy(asio::buffer(self->packet_), self->read_buffer_.cdata());
            self->read_buffer_.consume(self->read_buffer_.size());
            self->handle_packet();
        });
    }

    void handle_packet() {
        if (packet_.size() < PacketCodec::header_size) {
            close();
            return;
        }

        std::array<std::uint8_t, PacketCodec::header_size> header_bytes{};
        std::copy_n(packet_.begin(), PacketCodec::header_size, header_bytes.begin());
        const auto header = PacketCodec::decode_header(header_bytes);
        if (!header || header->packet_size != packet_.size() || header->packet_size > max_packet_size) {
            close();
            return;
        }

        refresh_heartbeat();
        const auto* body = packet_.data() + PacketCodec::header_size;
        const auto body_size = packet_.size() - PacketCodec::header_size;

        if (header->message_id != doudizhu::C2S_LOGIN && !player_id_.empty() && notifier_token_ &&
            !notifier_->is_current(player_id_, *notifier_token_)) {
            close();
            return;
        }

        switch (header->message_id) {
        case doudizhu::C2S_LOGIN: {
            doudizhu::C2SLogin request;
            if (!request.ParseFromArray(body, static_cast<int>(body_size))) {
                close();
                return;
            }
            const auto requested_player_id = request.player_id();
            auto self = shared_from_this();
            asio::post(backend_pool_, [self, requested_player_id] {
                const auto result = self->backend_client_->login(requested_player_id);
                asio::post(self->stream_.get_executor(), [self, requested_player_id, result] {
                    doudizhu::S2CLoginRet response;
                    response.set_success(result.success);
                    response.set_player_id(result.success ? result.player_id : requested_player_id);
                    response.set_coins(result.coins);
                    response.set_message(result.success ? "login accepted" : result.message);
                    if (result.success) {
                        self->player_id_ = result.player_id;
                        self->room_manager_->mark_online(self->player_id_);
                        const auto weak_session = std::weak_ptr<WebSocketSession>(self);
                        self->notifier_token_ = self->notifier_->register_player(self->player_id_, [weak_session](std::uint16_t message_id, const std::string& body) {
                            if (const auto session = weak_session.lock()) {
                                session->send_packet(message_id, body);
                            }
                        });
                        const auto state = self->room_manager_->room_state(self->player_id_);
                        if (state) {
                            for (const auto& player : state->players) {
                                const auto player_state = self->room_manager_->room_state(player.player_id);
                                if (player_state) {
                                    self->notifier_->send(player.player_id,
                                                          doudizhu::S2C_ROOM_STATE,
                                                          encode_room_state(*player_state));
                                }
                            }
                        }
                    }
                    self->send_packet(doudizhu::S2C_LOGIN_RET, response.SerializeAsString());
                });
            });
            return;
        }
        case doudizhu::C2S_HEARTBEAT: {
            doudizhu::C2SHeartBeat request;
            if (!request.ParseFromArray(body, static_cast<int>(body_size))) {
                close();
                return;
            }
            doudizhu::S2CHeartBeatRet response;
            response.set_server_time(std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count());
            send_packet(doudizhu::S2C_HEARTBEAT_RET, response.SerializeAsString());
            return;
        }
        case doudizhu::C2S_QUICK_MATCH: {
            doudizhu::C2SQuickMatch request;
            if (!request.ParseFromArray(body, static_cast<int>(body_size))) {
                close();
                return;
            }
            doudizhu::S2CMatchRet response;
            if (player_id_.empty()) {
                response.set_success(false);
                response.set_message("login required");
            } else {
                const auto result = room_manager_->match(player_id_);
                response.set_success(true);
                response.set_room_id(result.room_id);
                response.set_seat(result.seat);
                response.set_game_started(result.game_started);
                response.set_message(result.game_started ? "game ready" : "waiting for players");
                send_packet(doudizhu::S2C_MATCH_RET, response.SerializeAsString());
                if (result.game_start) {
                    for (std::size_t seat = 0; seat < result.game_start->hands.size(); ++seat) {
                        doudizhu::S2CDealCards deal;
                        deal.set_seat(static_cast<std::uint32_t>(seat));
                        deal.set_current_seat(result.game_start->current_seat);
                        for (const auto& card : result.game_start->hands[seat]) {
                            deal.add_cards(game::CardCodec::encode(card));
                        }
                        notifier_->send(result.game_start->player_ids[seat],
                                        doudizhu::S2C_DEAL_CARDS,
                                        deal.SerializeAsString());

                        const auto state = room_manager_->room_state(result.game_start->player_ids[seat]);
                        if (state) {
                            notifier_->send(result.game_start->player_ids[seat],
                                            doudizhu::S2C_ROOM_STATE,
                                            encode_room_state(*state));
                        }
                    }
                }
                return;
            }
            send_packet(doudizhu::S2C_MATCH_RET, response.SerializeAsString());
            return;
        }
        case doudizhu::C2S_CALL_LANDLORD: {
            doudizhu::C2SCallLandlord request;
            if (!request.ParseFromArray(body, static_cast<int>(body_size))) {
                close();
                return;
            }
            const auto result = room_manager_->call_landlord(player_id_, request.call());
            if (!result) {
                close();
                return;
            }
            doudizhu::S2CCallLandlordRet response;
            response.set_accepted(result->accepted);
            response.set_current_seat(result->current_seat);
            response.set_game_started(result->game_started);
            response.set_actor_seat(result->actor_seat);
            response.set_called(result->called);
            if (result->landlord_seat) {
                response.set_landlord_seat(*result->landlord_seat);
            }
            if (result->accepted) {
                for (const auto& player : result->player_ids) {
                    notifier_->send(player, doudizhu::S2C_CALL_LANDLORD_RET, response.SerializeAsString());
                    const auto state = room_manager_->room_state(player);
                    if (state) notifier_->send(player, doudizhu::S2C_ROOM_STATE, encode_room_state(*state));
                }
            } else {
                send_packet(doudizhu::S2C_CALL_LANDLORD_RET, response.SerializeAsString());
            }
            return;
        }
        case doudizhu::C2S_PLAY_CARDS: {
            doudizhu::C2SPlayCards request;
            if (!request.ParseFromArray(body, static_cast<int>(body_size))) {
                close();
                return;
            }
            std::vector<game::Card> cards;
            cards.reserve(request.cards_size());
            for (const auto encoded : request.cards()) {
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
            doudizhu::S2CPlayCardsRet response;
            response.set_accepted(result->accepted);
            response.set_current_seat(result->current_seat);
            response.set_game_over(result->game_over);
            send_packet(doudizhu::S2C_PLAY_CARDS_RET, response.SerializeAsString());
            if (result->accepted) {
                doudizhu::S2CPlayBroadcast broadcast;
                broadcast.set_seat(result->actor_seat);
                broadcast.set_current_seat(result->current_seat);
                for (const auto& card : cards) {
                    broadcast.add_cards(game::CardCodec::encode(card));
                }
                for (const auto& player : result->player_ids) {
                    notifier_->send(player, doudizhu::S2C_PLAY_BROADCAST, broadcast.SerializeAsString());
                    const auto state = room_manager_->room_state(player);
                    if (state) notifier_->send(player, doudizhu::S2C_ROOM_STATE, encode_room_state(*state));
                }
            }
            if (result->game_over) {
                for (const auto& settlement : result->settlements) {
                    doudizhu::S2CSettleResult settle_ret;
                    settle_ret.set_win(settlement.result == "WIN");
                    settle_ret.set_coin_change(settlement.coin_change);
                    notifier_->send(settlement.player_id,
                                    doudizhu::S2C_SETTLE_RESULT,
                                    settle_ret.SerializeAsString());
                    auto backend_client = backend_client_;
                    asio::post(backend_pool_, [backend_client, settlement] {
                        backend_client->settle(settlement.game_id, settlement.player_id, settlement.coin_change, settlement.result, settlement.duration_seconds);
                    });
                }
            }
            return;
        }
        case doudizhu::C2S_PASS: {
            doudizhu::C2SPass request;
            if (!request.ParseFromArray(body, static_cast<int>(body_size))) {
                close();
                return;
            }
            const auto result = room_manager_->pass(player_id_);
            if (!result) {
                close();
                return;
            }
            doudizhu::S2CPassRet response;
            response.set_accepted(result->accepted);
            response.set_current_seat(result->current_seat);
            response.set_game_over(result->game_over);
            send_packet(doudizhu::S2C_PASS_RET, response.SerializeAsString());
            if (result->accepted) {
                doudizhu::S2CPlayBroadcast broadcast;
                broadcast.set_seat(result->actor_seat);
                broadcast.set_passed(true);
                broadcast.set_current_seat(result->current_seat);
                for (const auto& player : result->player_ids) {
                    notifier_->send(player, doudizhu::S2C_PLAY_BROADCAST, broadcast.SerializeAsString());
                    const auto state = room_manager_->room_state(player);
                    if (state) notifier_->send(player, doudizhu::S2C_ROOM_STATE, encode_room_state(*state));
                }
            }
            return;
        }
        case doudizhu::C2S_RECONNECT: {
            doudizhu::C2SReconnect request;
            if (!request.ParseFromArray(body, static_cast<int>(body_size))) {
                close();
                return;
            }
            const auto result = room_manager_->reconnect_info(player_id_);
            if (!result) {
                close();
                return;
            }
            doudizhu::S2CReconnectSync response;
            response.set_room_id(result->room_id);
            response.set_seat(result->seat);
            response.set_phase(static_cast<std::uint32_t>(result->phase));
            response.set_current_seat(result->current_seat);
            if (result->landlord_seat) {
                response.set_landlord_seat(*result->landlord_seat);
            }
            for (const auto& card : result->hand) {
                response.add_cards(game::CardCodec::encode(card));
            }
            send_packet(doudizhu::S2C_RECONNECT_SYNC, response.SerializeAsString());
            return;
        }
        default:
            close();
            return;
        }
    }

    void send_packet(std::uint16_t message_id, const std::string& body) {
        const auto packet_size = PacketCodec::header_size + body.size();
        if (packet_size > max_packet_size) {
            close();
            return;
        }

        const auto header = PacketCodec::encode_header(
            PacketHeader{static_cast<std::uint32_t>(packet_size), message_id});
        std::vector<std::uint8_t> packet(packet_size);
        std::copy(header.begin(), header.end(), packet.begin());
        std::copy(body.begin(), body.end(), packet.begin() + PacketCodec::header_size);

        const auto start_write = write_queue_.empty();
        write_queue_.push_back(std::move(packet));
        if (start_write) {
            write_next();
        }
    }

    void write_next() {
        auto self = shared_from_this();
        stream_.async_write(asio::buffer(write_queue_.front()), [self](const boost::system::error_code& error, std::size_t) {
            if (error) {
                self->close();
                return;
            }

            self->write_queue_.pop_front();
            if (!self->write_queue_.empty()) {
                self->write_next();
            } else if (!self->read_in_progress_) {
                self->read_next();
            }
        });
    }

    void refresh_heartbeat() {
        heartbeat_timer_.expires_after(std::chrono::seconds(5));
        auto self = shared_from_this();
        heartbeat_timer_.async_wait([self](const boost::system::error_code& error) {
            if (!error) {
                self->close();
            }
        });
    }

    void close() {
        if (closed_) {
            return;
        }
        closed_ = true;
        heartbeat_timer_.cancel();
        if (!player_id_.empty() && notifier_token_ && notifier_->is_current(player_id_, *notifier_token_)) {
            room_manager_->mark_offline(player_id_, std::chrono::steady_clock::now());
            const auto state = room_manager_->room_state(player_id_);
            if (state) {
                const auto snapshot = encode_room_state(*state);
                for (const auto& player : state->players) {
                    notifier_->send(player.player_id, doudizhu::S2C_ROOM_STATE, snapshot);
                }
            }
            auto backend_client = backend_client_;
            const auto player_id = player_id_;
            asio::post(backend_pool_, [backend_client, player_id] {
                backend_client->mark_offline(player_id);
            });
            if (notifier_token_) {
                notifier_->unregister_player(player_id_, *notifier_token_);
            }
        }
        boost::system::error_code ignored;
        stream_.next_layer().shutdown(tcp::socket::shutdown_both, ignored);
        stream_.next_layer().close(ignored);
    }

    websocket::stream<tcp::socket> stream_;
    beast::flat_buffer read_buffer_;
    asio::steady_timer heartbeat_timer_;
    std::vector<std::uint8_t> packet_;
    std::deque<std::vector<std::uint8_t>> write_queue_;
    bool read_in_progress_ = false;
    std::string player_id_;
    std::optional<std::uint64_t> notifier_token_;
    bool closed_ = false;
    std::shared_ptr<RoomManager> room_manager_;
    std::shared_ptr<BackendClient> backend_client_;
    std::shared_ptr<PlayerNotifier> notifier_;
    asio::thread_pool& backend_pool_;
};

}  // namespace

WebSocketServer::WebSocketServer(asio::io_context& io_context,
                                 std::uint16_t port,
                                 std::shared_ptr<RoomManager> room_manager,
                                 std::shared_ptr<BackendClient> backend_client,
                                 std::shared_ptr<PlayerNotifier> notifier,
                                 asio::thread_pool& backend_pool)
    : acceptor_(io_context, tcp::endpoint(tcp::v4(), port)),
      room_manager_(std::move(room_manager)),
      backend_client_(std::move(backend_client)),
      notifier_(std::move(notifier)),
      backend_pool_(backend_pool) {
}

void WebSocketServer::start() {
    accept_next();
}

void WebSocketServer::accept_next() {
    acceptor_.async_accept([this](const boost::system::error_code& error, tcp::socket socket) {
        if (!error) {
            std::make_shared<WebSocketSession>(std::move(socket), room_manager_, backend_client_, notifier_, backend_pool_)->start();
        }
        accept_next();
    });
}

}  // namespace gateway
