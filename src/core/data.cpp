#include "data.hpp"

#include <cmath>

namespace gf {
namespace {

constexpr int L = 40, M = 70, H = 110;

Move mv(const char* n, int p) { return Move(n, p); }
Move util(const char* n) { return Move(n, 0); }

struct Line {
    const char* names[3];
    int stages;
    int hp[3], atk[3], def[3], spd[3];
    bool starter;
};

std::vector<Species> build() {
    std::vector<Species> v;
    auto add = [&](const char* name, int line, int stage, int hp, int atk, int def, int spd, std::array<Move, 4> m,
                   bool rare = false, bool starter = false) {
        Species s;
        s.id = static_cast<int>(v.size());
        s.name = name;
        s.line = line;
        s.stage = stage;
        s.hp = hp; s.atk = atk; s.def = def; s.spd = spd;
        s.moves = std::move(m);
        s.rare = rare;
        s.starter = starter;
        v.push_back(std::move(s));
    };
    auto evo = [&](int from, int to, int level) { v[from].evolvesTo = to; v[from].evolveLevel = level; };

    // ---- Starter line S1: Stalwart (tank / stall) ----
    add("Mossling", 0, 1, 4, 2, 3, 1, {
        mv("Leaf Slap", L),
        util("Sap Root").CD(3).T(Tgt::Self).Me(EK::Regen, 3),
        util("Bramble Coat").CD(3).T(Tgt::Self).Me(EK::Thorns, 3),
        util("Sprout Nap").CD(4).T(Tgt::Self).Me(EK::Heal, 30).Me(EK::Slow, 1)}, false, true);
    add("Thornback", 0, 2, 4, 3, 4, 1, {
        mv("Spike Roll", M).Fx(EK::Wound, 2, 0, 30),
        util("Ironbark").CD(3).T(Tgt::Self).Me(EK::Armor, 3),
        mv("Barbed Embrace", M).CD(3).Fx(EK::Trap, 2),
        util("Photosynth").CD(4).T(Tgt::Self).Me(EK::Heal, 30)});
    add("Grovewarden", 0, 3, 5, 3, 5, 1, {
        mv("Ancient Crush", H).CD(2).Me(EK::Slow, 1),
        util("Verdant Bulwark").CD(3).T(Tgt::Self).Me(EK::Shield, 20).Me(EK::Thorns, 2),
        util("Overgrowth").CD(4).T(Tgt::Self).Me(EK::Regen, 4).Me(EK::Cleanse),
        mv("Warden's Judgment", M).CD(3).Special(Sp::WardenBonus)});
    evo(0, 1, 16); evo(1, 2, 32);

    // ---- Starter line S2: Swift (speed / tempo) ----
    add("Zapling", 1, 1, 2, 3, 1, 5, {
        mv("Static Nip", L).Prio(),
        util("Hyper Dash").CD(3).T(Tgt::Self).Me(EK::Haste, 3),
        mv("Jolt Rattle", L).CD(3).Fx(EK::Daze, 2),
        util("Quick Charge").CD(3).T(Tgt::Self).Prio().Me(EK::Focus)}, false, true);
    add("Zephox", 1, 2, 3, 3, 2, 5, {
        mv("Slipstream Strike", M).Special(Sp::SlipstreamBonus),
        util("Afterimage").CD(3).T(Tgt::Self).Prio().Me(EK::Evade),
        mv("Spark Chain", 0).CD(3).H({L, L}).Rand().PerHit().Fx(EK::Daze, 1, 0, 20),
        util("Windup Zip").CD(3).T(Tgt::Self).Me(EK::Haste, 2).Me(EK::Focus)});
    add("Tempestail", 1, 3, 3, 4, 2, 5, {
        mv("Thunder Lash", H).CD(2).IgnoreDef(30),
        util("Eye of the Storm").CD(4).T(Tgt::Self).Me(EK::Evade).Me(EK::Empower, 60, 1),
        mv("Rolling Thunder", 0).CD(2).H({30, 50, 70}),
        util("Gale Veil").CD(3).T(Tgt::Self).Me(EK::Haste, 3).Me(EK::Cleanse)});
    evo(3, 4, 16); evo(4, 5, 32);

    // ---- Starter line S3: Ember (burst / setup) ----
    add("Cindrel", 2, 1, 2, 4, 1, 3, {
        mv("Ember Snap", L).Fx(EK::Wound, 2, 0, 25),
        util("Kindle").CD(3).T(Tgt::Self).Me(EK::Focus),
        mv("Flare Burst", H).CD(2).Recoil(10),
        util("Smolder Aura").CD(4).Fx(EK::Wound, 3).Fx(EK::Mark)}, false, true);
    add("Ashwolf", 2, 2, 3, 4, 2, 3, {
        mv("Cinder Fang", M).Fx(EK::Wound, 2, 0, 30),
        util("Ashen Howl").CD(3).T(Tgt::Self).Me(EK::AtkMod, 30, 3),
        util("Scorched Earth").CD(4).Fx(EK::Wound, 3).Fx(EK::Slow, 2),
        mv("Backdraft", H).CD(2).Special(Sp::BackdraftBonus)});
    add("Pyrelord", 2, 3, 3, 5, 2, 3, {
        mv("Inferno Crown", H).CD(2).Fx(EK::Wound, 2),
        util("Phoenix Pact").CD(5).T(Tgt::Self).Me(EK::SelfDamage, 25).Me(EK::Empower, 80, 2),
        mv("Ember Rain", 0).CD(2).H({L, L, L}).Rand().PerHit().Fx(EK::Wound, 2, 0, 25),
        util("Molten Guard").CD(3).T(Tgt::Self).Me(EK::CounterHit, M, 2)});
    evo(6, 7, 16); evo(7, 8, 32);

    // ---- L1 wall / counter ----
    add("Pebblet", 3, 1, 3, 2, 4, 1, {
        mv("Rock Toss", L),
        util("Curl Up").CD(3).T(Tgt::Self).Me(EK::Armor, 2).Me(EK::Counter, 15),
        mv("Gravel Spray", L).CD(3).T(Tgt::Spread).Fx(EK::Daze, 1),
        util("Settle In").CD(3).T(Tgt::Self).Me(EK::Heal, 15).Me(EK::Shield, 10)});
    add("Boulderon", 3, 2, 4, 3, 5, 1, {
        mv("Landslide", H).CD(2).Fx(EK::Slow, 2),
        util("Stonewall Stance").CD(3).T(Tgt::Self).Me(EK::Counter, 20),
        mv("Tremor Step", M).CD(3).Fx(EK::Seal),
        util("Bedrock Resolve").CD(4).T(Tgt::Self).Me(EK::Shield, 30).Me(EK::Slow, 1)});
    evo(9, 10, 20);

    // ---- L2 healer ----
    add("Dewdrip", 4, 1, 3, 1, 2, 3, {
        mv("Bubble Pop", L),
        util("Mending Mist").CD(3).T(Tgt::Ally).Fx(EK::Heal, 20),
        util("Rinse").CD(3).T(Tgt::Ally).Fx(EK::Cleanse).Fx(EK::Regen, 2),
        util("Slick Skin").CD(3).T(Tgt::Self).Me(EK::Evade, 0, 0, 50)});
    add("Tidalisk", 4, 2, 4, 2, 3, 3, {
        mv("Crashing Wave", H).CD(2).Me(EK::Cleanse),
        util("Healing Current").CD(3).T(Tgt::Ally).Fx(EK::Heal, 35),
        mv("Undertow", M).Fx(EK::Slow, 2),
        util("Tidal Barrier").CD(4).T(Tgt::Ally).Fx(EK::Shield, 25).Fx(EK::Regen, 2)});
    evo(11, 12, 20);

    // ---- L3 debuffer ----
    add("Gloomoth", 5, 1, 2, 2, 2, 4, {
        mv("Dust Flutter", L).Fx(EK::Daze, 1),
        util("Hex Mark").CD(3).Fx(EK::Mark),
        mv("Sap Scales", M).CD(3).Drain(50),
        util("Lullaby Wings").CD(4).Fx(EK::Slow, 3)});
    add("Dusklady", 5, 2, 3, 3, 2, 4, {
        util("Doom Whisper").CD(4).Fx(EK::Doom, 3),
        util("Silken Seal").CD(3).Fx(EK::Seal),
        mv("Nightfall Wings", M).CD(3).Fx(EK::Mark).Fx(EK::Daze, 1),
        util("Dark Veil").CD(3).T(Tgt::Self).Me(EK::Evade).Me(EK::Haste, 1)});
    evo(13, 14, 20);

    // ---- L4 hit-and-run ----
    add("Gustling", 6, 1, 2, 3, 1, 4, {
        mv("Feather Cut", L).Prio(),
        mv("Wind Hop", M).CD(2).Pivot(),
        mv("Updraft Dive", H).CD(3).Charge(true),
        util("Tailwind").CD(4).T(Tgt::Ally).Fx(EK::Haste, 3)});
    add("Cyclonix", 6, 2, 3, 4, 2, 4, {
        mv("Cyclone Strike", M).Pivot(),
        util("Whirl Away").CD(3).Fx(EK::ForceSwitch),
        mv("Razor Gale", H).CD(2).Crit(25),
        util("Eye Dart").CD(3).T(Tgt::Self).Me(EK::Evade).Me(EK::Focus)});
    evo(15, 16, 20);

    // ---- L5 support ----
    add("Glimmerling", 7, 1, 3, 1, 2, 3, {
        mv("Sparkle Tap", L),
        util("Prism Shield").CD(3).T(Tgt::Ally).Fx(EK::Shield, 15),
        util("Lucky Glint").CD(3).T(Tgt::Ally).Fx(EK::Focus).Fx(EK::NoMiss),
        util("Dazzle").CD(3).Fx(EK::Daze, 2)});
    add("Prismaw", 7, 2, 4, 2, 3, 3, {
        mv("Refract Beam", M).CD(2).IgnoreDefenses(),
        util("Crystal Aegis").CD(4).T(Tgt::Ally).Fx(EK::Shield, 30).Fx(EK::Thorns, 2),
        util("Spectrum Boost").CD(4).T(Tgt::Ally).Fx(EK::AtkMod, 20, 3).Fx(EK::DefMod, 20, 3).Fx(EK::SpdMod, 20, 3),
        util("Mirror Shard").CD(3).T(Tgt::Self).Me(EK::ReflectDebuff)});
    evo(17, 18, 20);

    // ---- L6 trapper ----
    add("Burrowby", 8, 1, 3, 2, 3, 2, {
        mv("Claw Scratch", L).Fx(EK::Wound, 2, 0, 20),
        mv("Dig In", M).CD(3).Charge(true),
        util("Pitfall").CD(3).Fx(EK::Trap, 2),
        util("Dirt Cloud").CD(3).T(Tgt::Spread).Fx(EK::Daze, 1)});
    add("Tunnelord", 8, 2, 4, 3, 4, 2, {
        mv("Cave-In", H).CD(2).Fx(EK::Trap, 1),
        util("Spike Pit").CD(3).Fx(EK::Wound, 3).Fx(EK::Trap, 2),
        mv("Rumble", M).CD(2).T(Tgt::Spread).Fx(EK::Slow, 1),
        util("Fortify Burrow").CD(4).T(Tgt::Self).Me(EK::Armor, 3).Me(EK::Regen, 2)});
    evo(19, 20, 20);

    // ---- L7 control ----
    add("Frostwisp", 9, 1, 2, 2, 2, 3, {
        mv("Chill Breath", L).Fx(EK::Slow, 1),
        util("Frost Veil").CD(3).T(Tgt::Self).Me(EK::Armor, 2),
        mv("Icicle Prick", M).CD(3).Fx(EK::Slow, 2, 0, 40),
        util("Snow Drift").CD(5).Fx(EK::Stun)});
    add("Glacierra", 9, 2, 4, 3, 3, 2, {
        mv("Glacial Slam", H).CD(2).Fx(EK::Slow, 2),
        util("Flash Freeze").CD(5).Fx(EK::Stun),
        util("Permafrost").CD(4).T(Tgt::Self).Me(EK::Armor, 3).Me(EK::SlowAttackers, 3),
        util("Blizzard Veil").CD(4).T(Tgt::Spread).Fx(EK::Daze, 2)});
    evo(21, 22, 20);

    // ---- L8 trickster ----
    add("Mirrorat", 10, 1, 2, 2, 2, 4, {
        mv("Cheeky Bite", L),
        util("Mimic Move").CD(2).Mimic(70),
        util("Decoy").CD(3).T(Tgt::Self).Me(EK::Shield, 15).Me(EK::Evade),
        util("Pilfer").CD(3).Special(Sp::StealBuff)});
    add("Reflectrix", 10, 2, 3, 3, 2, 4, {
        util("Hall of Mirrors").CD(4).T(Tgt::Self).Me(EK::ReflectDamage),
        util("Copycat Strike").CD(2).Mimic(100),
        util("Inversion").CD(5).Special(Sp::InvertBuffs),
        mv("Trick Step", M).CD(3).Prio().Fx(EK::Daze, 1)});
    evo(23, 24, 20);

    // ---- Rares ----
    add("Chronowl", 11, 1, 3, 3, 2, 4, {
        util("Stitch in Time").CD(4).T(Tgt::Self).Special(Sp::ResetCooldowns),
        util("Time Skip").CD(3).T(Tgt::Self).Prio().Me(EK::PrioNext),
        util("Fateful Hoot").CD(4).Fx(EK::Doom, 2),
        util("Hourglass Gaze").CD(3).Fx(EK::Slow, 2).Fx(EK::Mark)}, true);
    add("Ironmite", 12, 1, 4, 3, 5, 1, {
        mv("Rivet Bite", M),
        util("Plate Up").T(Tgt::Self).Me(EK::Plate),
        util("Rust Cloud").CD(3).Fx(EK::DefMod, -30, 3),
        mv("Cannonball Roll", H).CD(3).Special(Sp::CannonballBonus)}, true);
    add("Sangrebat", 13, 1, 3, 3, 2, 4, {
        mv("Fang Sip", M).Drain(50),
        util("Blood Pact").CD(4).T(Tgt::Self).Me(EK::SelfDamage, 20).Me(EK::AtkMod, 50, 3),
        util("Crimson Mist").CD(3).Fx(EK::Wound, 3).Me(EK::Leech, 3),
        util("Night Flit").CD(3).T(Tgt::Self).Me(EK::Evade).Me(EK::Heal, 10)}, true);
    add("Mimicube", 14, 1, 4, 3, 3, 2, {
        mv("Cubic Bump", L),
        util("Disguise").CD(4).Special(Sp::CopyMods),
        util("Gelatinous Absorb").CD(3).T(Tgt::Self).Me(EK::CounterAbsorb),
        mv("Shapeshift Slam", 0).CD(3).Special(Sp::ShapeshiftSlam)}, true);
    add("Ashenfeather", 15, 1, 3, 4, 2, 3, {
        mv("Cinder Wing", M),
        util("Rebirth Pyre").Once().T(Tgt::Self).Special(Sp::RebirthPyre),
        util("Ash Veil").CD(3).T(Tgt::Self).Me(EK::Shield, 15).Me(EK::Cleanse),
        mv("Solar Dive", H).CD(3).Charge(false).IgnoreDefenses()}, true);
    add("Kaleidra", 16, 1, 3, 4, 2, 3, {
        mv("Chaos Bolt", M).Special(Sp::ChaosBolt),
        util("Wild Card").CD(3).T(Tgt::Self).Special(Sp::WildCard),
        mv("Prismatic Shatter", M).CD(3).Special(Sp::PrismaticShatter),
        util("Anarchy").CD(5).T(Tgt::Self).Special(Sp::Anarchy)}, true);
    return v;
}

}  // namespace

const std::vector<Species>& allSpecies() {
    static const std::vector<Species> s = build();
    return s;
}

const Species& species(int id) { return allSpecies().at(static_cast<size_t>(id)); }

Stats statsFor(const Species& s, int level) {
    auto stat = [&](int pips) {
        const double base = pips * 25.0;
        return static_cast<int>((2 * base + 31) * level / 100.0 + 5);
    };
    const double hpBase = s.hp * 25.0;
    Stats st;
    st.hp = static_cast<int>((2 * hpBase + 31) * level / 100.0 + level + 10);
    st.atk = stat(s.atk);
    st.def = stat(s.def);
    st.spd = stat(s.spd);
    return st;
}

int xpToNext(int level) { return static_cast<int>(20.0 * std::pow(static_cast<double>(level), 1.6)); }

std::vector<int> baseSpecies() {
    std::vector<int> out;
    std::vector<bool> isEvolved(allSpecies().size(), false);
    for (const auto& s : allSpecies())
        if (s.evolvesTo >= 0) isEvolved[static_cast<size_t>(s.evolvesTo)] = true;
    for (const auto& s : allSpecies())
        if (!isEvolved[static_cast<size_t>(s.id)]) out.push_back(s.id);
    return out;
}

std::vector<int> starterSpecies() {
    std::vector<int> out;
    for (const auto& s : allSpecies())
        if (s.starter) out.push_back(s.id);
    return out;
}

}  // namespace gf
