#include "game/card_rules.hpp"

#include <algorithm>
#include <map>
#include <optional>

namespace game {
namespace {

std::uint8_t rank_value(Rank rank) {
    return static_cast<std::uint8_t>(rank);
}

bool consecutive(const std::map<std::uint8_t, int>& counts) {
    if (counts.empty() || counts.rbegin()->first > rank_value(Rank::Ace)) {
        return false;
    }

    auto current = counts.begin();
    auto next = std::next(current);
    while (next != counts.end()) {
        if (next->first != current->first + 1) {
            return false;
        }
        ++current;
        ++next;
    }
    return true;
}

std::optional<Rank> airplane_main_rank(const std::map<std::uint8_t, int>& counts,
                                       std::size_t triple_count) {
    const auto first_rank = rank_value(Rank::Three);
    const auto last_rank = rank_value(Rank::Ace);
    for (auto start = first_rank; start + triple_count - 1 <= last_rank; ++start) {
        bool valid = true;
        for (std::size_t offset = 0; offset < triple_count; ++offset) {
            const auto entry = counts.find(start + offset);
            if (entry == counts.end() || entry->second != 3) {
                valid = false;
                break;
            }
        }
        if (valid) {
            return static_cast<Rank>(start + triple_count - 1);
        }
    }
    return std::nullopt;
}

}  // namespace

Play CardRules::analyze(const std::vector<Card>& cards) {
    if (cards.empty()) {
        return {};
    }

    std::map<std::uint8_t, int> counts;
    for (const auto& card : cards) {
        ++counts[rank_value(card.rank)];
    }

    const auto highest_rank = static_cast<Rank>(counts.rbegin()->first);
    if (cards.size() == 2 && counts.find(rank_value(Rank::SmallJoker)) != counts.end() &&
        counts.find(rank_value(Rank::BigJoker)) != counts.end()) {
        return {CardPattern::Rocket, Rank::BigJoker, cards.size()};
    }

    if (counts.size() == 1) {
        switch (cards.size()) {
        case 1:
            return {CardPattern::Single, highest_rank, cards.size()};
        case 2:
            return {CardPattern::Pair, highest_rank, cards.size()};
        case 3:
            return {CardPattern::Triple, highest_rank, cards.size()};
        case 4:
            return {CardPattern::Bomb, highest_rank, cards.size()};
        default:
            return {};
        }
    }

    const auto triple = std::find_if(counts.begin(), counts.end(),
        [](const auto& entry) { return entry.second == 3; });
    if (cards.size() == 4 && triple != counts.end()) {
        return {CardPattern::TripleWithSingle, static_cast<Rank>(triple->first), cards.size()};
    }
    if (cards.size() == 5 && triple != counts.end() && counts.size() == 2) {
        const auto pair = std::find_if(counts.begin(), counts.end(),
            [](const auto& entry) { return entry.second == 2; });
        if (pair != counts.end()) {
            return {CardPattern::TripleWithPair, static_cast<Rank>(triple->first), cards.size()};
        }
    }

    if (cards.size() >= 5 && counts.size() == cards.size() && consecutive(counts)) {
        return {CardPattern::Straight, highest_rank, cards.size()};
    }

    if (cards.size() >= 6 && cards.size() % 2 == 0 && consecutive(counts) &&
        std::all_of(counts.begin(), counts.end(),
            [](const auto& entry) { return entry.second == 2; })) {
        return {CardPattern::ConsecutivePairs, highest_rank, cards.size()};
    }

    if (cards.size() >= 6 && cards.size() % 3 == 0 && consecutive(counts) &&
        std::all_of(counts.begin(), counts.end(),
            [](const auto& entry) { return entry.second == 3; })) {
        return {CardPattern::Airplane, highest_rank, cards.size()};
    }

    if (cards.size() >= 8 && cards.size() % 4 == 0) {
        const auto triple_count = cards.size() / 4;
        const auto main_rank = airplane_main_rank(counts, triple_count);
        if (main_rank) {
            auto remaining = counts;
            for (auto rank = static_cast<std::uint8_t>(*main_rank) - triple_count + 1;
                 rank <= static_cast<std::uint8_t>(*main_rank);
                 ++rank) {
                remaining.erase(rank);
            }
            std::size_t remaining_cards = 0;
            for (const auto& [_, count] : remaining) {
                remaining_cards += count;
            }
            if (remaining_cards == triple_count) {
                return {CardPattern::AirplaneWithSingles, *main_rank, cards.size()};
            }
        }
    }

    if (cards.size() >= 10 && cards.size() % 5 == 0) {
        const auto triple_count = cards.size() / 5;
        const auto main_rank = airplane_main_rank(counts, triple_count);
        if (main_rank) {
            auto remaining = counts;
            for (auto rank = static_cast<std::uint8_t>(*main_rank) - triple_count + 1;
                 rank <= static_cast<std::uint8_t>(*main_rank);
                 ++rank) {
                remaining.erase(rank);
            }
            if (remaining.size() == triple_count &&
                std::all_of(remaining.begin(), remaining.end(),
                    [](const auto& entry) { return entry.second == 2; })) {
                return {CardPattern::AirplaneWithPairs, *main_rank, cards.size()};
            }
        }
    }

    return {};
}

}  // namespace game
