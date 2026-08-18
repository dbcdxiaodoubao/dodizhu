#include "game/game_round.hpp"

#include <algorithm>

namespace game {

GameRound::GameRound(std::array<std::string, 3> player_ids)
    : player_ids_(std::move(player_ids)) {
}

void GameRound::start(std::mt19937& random) {
    deal_ = deck_.deal(random);
    phase_ = GamePhase::CallLandlord;
    current_seat_ = 0;
    call_count_ = 0;
    last_caller_.reset();
    landlord_seat_.reset();
    winner_seat_.reset();
    last_play_.reset();
    last_play_seat_.reset();
    pass_count_ = 0;
}

bool GameRound::call_landlord(std::uint8_t seat, bool call, std::mt19937& random) {
    if (phase_ != GamePhase::CallLandlord || seat != current_seat_) {
        return false;
    }

    if (call) {
        last_caller_ = seat;
    }
    ++call_count_;

    if (call_count_ == 3) {
        if (!last_caller_) {
            start(random);
            return true;
        }

        landlord_seat_ = last_caller_;
        auto& landlord_hand = deal_.hands[*landlord_seat_];
        landlord_hand.insert(landlord_hand.end(), deal_.bottom_cards.begin(), deal_.bottom_cards.end());
        phase_ = GamePhase::PlayCards;
        current_seat_ = *landlord_seat_;
        last_play_.reset();
        last_play_seat_.reset();
        pass_count_ = 0;
        return true;
    }

    current_seat_ = static_cast<std::uint8_t>((current_seat_ + 1) % 3);
    return true;
}

bool GameRound::timeout(std::mt19937& random) {
    if (phase_ == GamePhase::CallLandlord) {
        return call_landlord(current_seat_, false, random);
    }

    if (phase_ != GamePhase::PlayCards) {
        return false;
    }

    if (last_play_ && last_play_seat_ != current_seat_) {
        return pass(current_seat_);
    }

    const auto& current_hand = hand(current_seat_);
    if (current_hand.empty()) {
        return false;
    }
    return play_cards(current_seat_, {current_hand.front()});
}

bool GameRound::play_cards(std::uint8_t seat, const std::vector<Card>& cards) {
    if (phase_ != GamePhase::PlayCards || seat != current_seat_ || cards.empty()) {
        return false;
    }

    const auto play = CardRules::analyze(cards);
    if (play.pattern == CardPattern::Invalid) {
        return false;
    }

    if (last_play_) {
        const auto can_beat =
            play.pattern == CardPattern::Rocket ||
            (play.pattern == CardPattern::Bomb && last_play_->pattern != CardPattern::Rocket &&
             (last_play_->pattern != CardPattern::Bomb ||
              static_cast<std::uint8_t>(play.main_rank) > static_cast<std::uint8_t>(last_play_->main_rank))) ||
            (play.pattern == last_play_->pattern && play.card_count == last_play_->card_count &&
             static_cast<std::uint8_t>(play.main_rank) > static_cast<std::uint8_t>(last_play_->main_rank));
        if (!can_beat) {
            return false;
        }
    }

    auto& player_hand = deal_.hands.at(seat);
    auto remaining_hand = player_hand;
    for (const auto& card : cards) {
        const auto held_card = std::find(remaining_hand.begin(), remaining_hand.end(), card);
        if (held_card == remaining_hand.end()) {
            return false;
        }
        remaining_hand.erase(held_card);
    }

    player_hand = std::move(remaining_hand);
    last_play_ = play;
    last_play_seat_ = seat;
    pass_count_ = 0;

    if (player_hand.empty()) {
        phase_ = GamePhase::Settled;
        winner_seat_ = seat;
        return true;
    }

    current_seat_ = static_cast<std::uint8_t>((seat + 1) % 3);
    return true;
}

bool GameRound::pass(std::uint8_t seat) {
    if (phase_ != GamePhase::PlayCards || seat != current_seat_ || !last_play_ ||
        last_play_seat_ == seat) {
        return false;
    }

    ++pass_count_;
    if (pass_count_ == 2) {
        current_seat_ = *last_play_seat_;
        last_play_.reset();
        last_play_seat_.reset();
        pass_count_ = 0;
        return true;
    }

    current_seat_ = static_cast<std::uint8_t>((seat + 1) % 3);
    return true;
}

GamePhase GameRound::phase() const {
    return phase_;
}

std::uint8_t GameRound::current_seat() const {
    return current_seat_;
}

std::optional<std::uint8_t> GameRound::landlord_seat() const {
    return landlord_seat_;
}

std::optional<std::uint8_t> GameRound::winner_seat() const {
    return winner_seat_;
}

const std::vector<Card>& GameRound::hand(std::uint8_t seat) const {
    return deal_.hands.at(seat);
}

}  // namespace game
