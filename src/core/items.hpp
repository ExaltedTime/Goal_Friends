// Held items: loot from dungeons. A friend holds at most one; it changes stats or adds a small battle effect.
#pragma once

namespace gf {

enum It : int {
    ItPowerBand, ItMindGem, ItGuardPlate, ItWardCharm, ItSwiftFeather, ItVitalBerry,  // common
    ItScopeLens, ItLeftovers, ItShieldSeed,                                          // rare
    ItFocusSash, ItLifeOrb, ItChampionBadge,                                         // epic
    ItCount
};

struct ItemInfo {
    const char* name;
    const char* desc;
    int rarity;  // 0 common, 1 rare, 2 epic
    int hp, atk, def, spa, spd, spe;  // stat boost in percent
};

const ItemInfo& itemInfo(int id);
const char* rarityName(int rarity);

}  // namespace gf
