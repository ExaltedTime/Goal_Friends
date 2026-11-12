// Human-readable descriptions of moves, shared by the in-game UI and the roster document generator.
#pragma once
#include <string>

#include "data.hpp"

namespace gf {

std::string effectText(const Effect& e);
std::string specialText(Sp sp);
std::string describeMove(const Move& m);          // e.g. "Medium physical hit · CD 3 · Wound 2 (30%)"
// A string that is identical for two moves exactly when they do the same thing (names ignored). Used to check roster variety.
std::string moveSignature(const Move& m);
const char* statLabel(int index);                 // 0..5 -> HP, ATK, DEF, SPA, SPD, SPE

}  // namespace gf
