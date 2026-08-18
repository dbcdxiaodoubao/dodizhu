#include "game/card_rules.hpp"

#include <algorithm>
#include <map>

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

    return {};
}

}  // namespace game
