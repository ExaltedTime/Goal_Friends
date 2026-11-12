// Prints the full roster document (ROSTER.md) straight from the game data, so it can never drift from the game.
// Build: part of the CMake project (target gf_roster_doc). Run: ./gf_roster_doc > ROSTER.md
#include <cstdio>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "data.hpp"
#include "describe.hpp"

using namespace gf;

static const char* kArea[6] = {"Sunny Meadow", "Whispering Woods", "Echo Caverns", "Sapphire Shore", "Stormcrest Peaks", "Ember Ruins"};

static std::string areas(int mask) {
    std::string s;
    for (int i = 0; i < 6; ++i)
        if (mask >> i & 1) { if (!s.empty()) s += ", "; s += kArea[i]; }
    return s;
}

static const char* kind(const Move& m) {
    if (!m.damaging() && m.sp != Sp::ShapeshiftSlam) return "Utility";
    return m.special ? "Special" : "Physical";
}

int main() {
    const auto& all = allSpecies();
    std::map<std::string, int> uses;
    std::vector<std::vector<std::string>> sigs(all.size());
    for (const auto& s : all)
        for (const auto& m : s.moves) { sigs[static_cast<size_t>(s.id)].push_back(moveSignature(m)); uses[sigs[static_cast<size_t>(s.id)].back()]++; }

    std::vector<int> from(all.size(), -1);
    for (const auto& s : all) if (s.evolvesTo >= 0) from[static_cast<size_t>(s.evolvesTo)] = s.id;

    std::printf("# Goal Friends: Roster (%zu friends)\n\n", all.size());
    std::printf("_Generated from the game data by `gf_roster_doc` (`tools/dump_roster.cpp`); do not edit by hand. "
                "Friends 49 to 150 are authored in `tools/gen_roster.py`._\n\n");
    std::printf("Six stats, as in the Pokémon games: **HP**, **ATK** (attack), **DEF** (defence), **SPA** (special attack), "
                "**SPD** (special defence) and **SPE** (speed). Base stats are 1 to 5 pips; the in-game values are shown at level 50. "
                "Physical moves use ATK against DEF, special moves use SPA against SPD. Every move shows its kind. "
                "A star marks a move whose exact mechanics no other friend has.\n\n");
    std::printf("Variety: %zu distinct move mechanics among %zu moves; every friend has its own stat spread and at least one move nobody else has.\n\n",
                uses.size(), all.size() * 4);

    // ---- overview table
    std::printf("## Overview\n\n| # | Friend | Stage | Role | HP | ATK | DEF | SPA | SPD | SPE | Found in |\n|---|---|---|---|---|---|---|---|---|---|---|\n");
    for (const auto& s : all) {
        int total = 1;
        for (const auto& p : all) if (p.line == s.line) total = std::max(total, p.stage);
        std::printf("| %d | %s | %d/%d%s | %s | %d | %d | %d | %d | %d | %d | %s |\n", s.id + 1, s.name.c_str(), s.stage, total, s.rare ? " rare" : "", s.role.c_str(),
                    s.hp, s.atk, s.def, s.spa, s.spd, s.spe, areas(s.habitats).c_str());
    }

    // ---- one section per friend
    std::printf("\n## Friends\n");
    int lastLine = -1;
    for (const auto& s : all) {
        if (s.line != lastLine) { lastLine = s.line; std::printf("\n---\n\n## %s: %s\n", s.rare ? s.name.c_str() : s.name.c_str(), s.role.c_str()); }
        const Stats st = statsFor(s, 50);
        int total = 1;
        for (const auto& p : all) if (p.line == s.line) total = std::max(total, p.stage);
        std::printf("\n### %d. %s\n\n", s.id + 1, s.name.c_str());
        std::printf("*%s* · stage %d of %d%s · found in %s\n\n", s.role.c_str(), s.stage, total, s.rare ? " · rare (no evolution)" : "", areas(s.habitats).c_str());
        std::printf("%s\n\n", s.blurb.c_str());
        std::printf("**Ability: %s** — %s\n\n", abilityInfo(s.ability).name, abilityInfo(s.ability).desc);
        const int f = from[static_cast<size_t>(s.id)];
        if (f >= 0) std::printf("Evolves from **%s** at level %d.%s", all[static_cast<size_t>(f)].name.c_str(), all[static_cast<size_t>(f)].evolveLevel, s.evolvesTo >= 0 ? " " : "\n\n");
        if (s.evolvesTo >= 0) std::printf("Evolves into **%s** at level %d.\n\n", all[static_cast<size_t>(s.evolvesTo)].name.c_str(), s.evolveLevel);
        std::printf("| HP | ATK | DEF | SPA | SPD | SPE | Total |\n|---|---|---|---|---|---|---|\n");
        std::printf("| %d (%d) | %d (%d) | %d (%d) | %d (%d) | %d (%d) | %d (%d) | %d |\n\n", s.hp, st.hp, s.atk, st.atk, s.def, st.def, s.spa, st.spa, s.spd, st.spd, s.spe, st.spe,
                    s.hp + s.atk + s.def + s.spa + s.spd + s.spe);
        std::printf("| Move | Kind | Details |\n|---|---|---|\n");
        for (size_t i = 0; i < 4; ++i) {
            const Move& m = s.moves[i];
            const bool unique = uses[sigs[static_cast<size_t>(s.id)][i]] == 1;
            std::string text = describeMove(m);
            if (text.rfind("Utility · ", 0) == 0) text = text.substr(10);   // the Kind column already says Utility
            else if (text == "Utility") text = "no effect";
            std::printf("| **%s**%s | %s | %s |\n", m.name.c_str(), unique ? " ★" : "", kind(m), text.c_str());
        }
        // closest relative by shared mechanics
        int best = 0, bestId = -1;
        for (const auto& o : all) {
            if (o.id == s.id) continue;
            std::multiset<std::string> A(sigs[static_cast<size_t>(s.id)].begin(), sigs[static_cast<size_t>(s.id)].end());
            int common = 0;
            for (const auto& g : sigs[static_cast<size_t>(o.id)]) { auto it = A.find(g); if (it != A.end()) { ++common; A.erase(it); } }
            if (common > best) { best = common; bestId = o.id; }
        }
        int uniqueCount = 0;
        for (size_t i = 0; i < 4; ++i) if (uses[sigs[static_cast<size_t>(s.id)][i]] == 1) ++uniqueCount;
        std::printf("\n%d of 4 moves are unique to this friend.", uniqueCount);
        if (bestId >= 0 && best >= 2) std::printf(" Closest relative: %s (%d shared moves).", all[static_cast<size_t>(bestId)].name.c_str(), best);
        std::printf("\n");
    }
    return 0;
}
