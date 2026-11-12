#include "ability.hpp"
#include <cstddef>

namespace gf {

const AbilityInfo& abilityInfo(Ab a) {
    static const AbilityInfo table[] = {
        {"None", "No ability."},
        {"Intimidate", "On entry, lowers the ATK and SPA of all foes by 20% for 3 turns."},
        {"Spotter's Eye", "On entry, marks the foe with the highest attack."},
        {"Download", "On entry, raises its own ATK and SPA by 25% for 3 turns."},
        {"Healing Aura", "On entry, heals all active allies by 10%."},
        {"Ward Up", "On entry, gains a shield worth 15% of its max HP."},
        {"Second Wind", "Heals 12% of its max HP when it enters the field."},
        {"Rough Skin", "Attackers lose 6% of their max HP when they hit this friend."},
        {"Static", "30% chance to Daze an attacker for 2 turns."},
        {"Flame Touch", "30% chance to Wound an attacker for 2 turns."},
        {"Cursed Body", "30% chance to Seal the attacker's last-used move."},
        {"Sturdy", "Once per battle, survives a lethal hit from full HP with 1 HP."},
        {"Dust Veil", "Attackers are Slowed for 1 turn."},
        {"Berserk", "Once, when brought to half HP, raises ATK and SPA by 30% for 3 turns."},
        {"Stamina", "Each hit taken raises DEF and SPD by 10% (up to 50%)."},
        {"Rattled", "Being hit raises SPE by 20% for 3 turns."},
        {"Aftermath", "If knocked out by an attack, the attacker loses 25% of its max HP."},
        {"Effect Spore", "30% chance to Wound, Daze or Slow an attacker."},
        {"Slippery", "15% chance to dodge each attack."},
        {"Technician", "Light hits deal 50% more damage."},
        {"Keen Eye", "+20% chance for any attack to be a critical hit (1.5x)."},
        {"Heavy Hitter", "Heavy hits deal 20% more damage."},
        {"Guts", "Deals 30% more damage while suffering a negative effect."},
        {"Hustle", "Deals 20% more damage, but 10% of attacks miss."},
        {"Skill Link", "Multi-hit moves deal 25% more damage."},
        {"Mold Breaker", "Attacks ignore the target's Armor and Shield."},
        {"Steady Aim", "Attacks never miss because of Daze."},
        {"Reckless", "Moves with recoil deal 25% more damage."},
        {"Analytic", "Deals 30% more damage to foes that have already moved this turn."},
        {"Moxie", "Knocking out a foe raises ATK and SPA by 25% for 3 turns."},
        {"Serene Grace", "Move effect chances are 50% higher."},
        {"Last Stand", "Deals 30% more damage at or below a third of its HP."},
        {"Mind Over Matter", "Special moves deal 20% more damage."},
        {"Brawn Power", "Physical moves deal 20% more damage."},
        {"Venom Touch", "25% chance for any hit to also Wound the target."},
        {"Hard Head", "Takes no recoil damage."},
        {"Fur Coat", "Takes 20% less damage from physical moves."},
        {"Spirit Ward", "Takes 20% less damage from special moves."},
        {"Multiscale", "Takes 35% less damage while at full HP."},
        {"Solid Rock", "Takes 25% less damage from Heavy hits."},
        {"Shell Armor", "Cannot be hit by critical hits."},
        {"Magic Guard", "Immune to Wound, Doom and leech effects from foes."},
        {"Speed Boost", "Gains 10% SPE at the end of every turn (up to 50%)."},
        {"Dew Skin", "Heals 6% of its max HP at the end of every turn."},
        {"Wound Heal", "Heals 8% of its max HP instead of taking Wound damage."},
        {"Shed Skin", "33% chance each turn to shed all negative effects."},
        {"Nightmare", "Foes suffering a negative effect lose 4% of their max HP each turn."},
        {"Quick Reload", "Move cooldowns tick down one extra turn every turn."},
        {"Regenerator", "Heals 25% of its max HP when it switches out."},
        {"Shadow Tag", "Foes cannot switch out voluntarily while this friend is on the field."},
        {"Run Away", "Cannot be trapped; can always switch out."},
        {"Pressure", "Foes' moves have +1 cooldown whenever they are used."},
        {"Oblivious", "Immune to Daze and Mark."},
        {"Insomnia", "Immune to Stun."},
        {"Clear Body", "Foes cannot lower this friend's stats or Slow it."},
        {"Keen Mind", "Immune to Seal and Taunt."},
        {"Magic Bounce", "Reflects the first negative effect aimed at it back at its source."},
        {"Contrary", "Stat changes affecting this friend are reversed."},
        {"Prankster", "Utility moves gain Priority."},
        {"Stall", "Always acts last in a turn."},
    };
    static_assert(sizeof(table) / sizeof(table[0]) == static_cast<size_t>(Ab::Count), "ability table out of sync with the enum");
    const int i = static_cast<int>(a);
    return table[i < 0 || i >= kAbilityCount ? 0 : i];
}

}  // namespace gf
