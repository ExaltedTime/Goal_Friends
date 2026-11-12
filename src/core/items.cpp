#include "items.hpp"

namespace gf {

const ItemInfo& itemInfo(int id) {
    static const ItemInfo table[ItCount] = {
        {"Power Band", "+12% ATK.", 0, 0, 12, 0, 0, 0, 0},
        {"Mind Gem", "+12% SPA.", 0, 0, 0, 0, 12, 0, 0},
        {"Guard Plate", "+12% DEF.", 0, 0, 0, 12, 0, 0, 0},
        {"Ward Charm", "+12% SPD.", 0, 0, 0, 0, 0, 12, 0},
        {"Swift Feather", "+12% SPE.", 0, 0, 0, 0, 0, 0, 12},
        {"Vital Berry", "+10% HP.", 0, 10, 0, 0, 0, 0, 0},
        {"Scope Lens", "+15% chance for attacks to be critical hits.", 1, 0, 0, 0, 0, 0, 0},
        {"Leftovers", "Heals 6% of max HP at the end of every turn.", 1, 0, 0, 0, 0, 0, 0},
        {"Shield Seed", "Starts every battle (and every entry) with a shield worth 15% of max HP.", 1, 0, 0, 0, 0, 0, 0},
        {"Focus Sash", "Once per battle, survives a lethal hit from full HP with 1 HP.", 2, 0, 0, 0, 0, 0, 0},
        {"Life Orb", "Attacks deal 20% more damage, but cost 5% of max HP.", 2, 0, 0, 0, 0, 0, 0},
        {"Champion Badge", "+6% to every stat.", 2, 6, 6, 6, 6, 6, 6},
    };
    return table[id < 0 || id >= ItCount ? 0 : id];
}

const char* rarityName(int r) { return r <= 0 ? "Common" : r == 1 ? "Rare" : "Epic"; }

}  // namespace gf
