// Passive abilities, one per friend (like the Pokémon games). The battle engine applies them at fixed hooks:
// entering the field, being hit, dealing damage, end of turn, switching, status application and move priority.
#pragma once
#include <cstdint>

namespace gf {

enum class Ab : uint8_t {
    None,
    // on entering the field
    Intimidate, SpottersEye, Download, HealingAura, WardUp, SecondWind,
    // when hit
    RoughSkin, Static, FlameTouch, CursedBody, Sturdy, DustVeil, Berserk, Stamina, Rattled, Aftermath, EffectSpore, Slippery,
    // dealing damage
    Technician, KeenEye, HeavyHitter, Guts, Hustle, SkillLink, MoldBreaker, SteadyAim, Reckless, Analytic, Moxie, SereneGrace,
    LastStand, MindOverMatter, BrawnPower, VenomTouch, HardHead,
    // taking damage
    FurCoat, SpiritWard, Multiscale, SolidRock, ShellArmor, MagicGuard,
    // end of turn
    SpeedBoost, DewSkin, WoundHeal, ShedSkin, Nightmare, QuickReload,
    // switching and trapping
    Regenerator, ShadowTag, RunAway, Pressure,
    // status immunities
    Oblivious, Insomnia, ClearBody, KeenMind, MagicBounce, Contrary,
    // turn order
    Prankster, Stall,
    Count
};

struct AbilityInfo {
    const char* name;
    const char* desc;
};

constexpr int kAbilityCount = static_cast<int>(Ab::Count);
const AbilityInfo& abilityInfo(Ab a);

}  // namespace gf
