#include "data.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <set>

namespace gf {
namespace {

constexpr int L = 40, M = 70, H = 110;

Move mv(const char* n, int p) { return Move(n, p); }
Move util(const char* n) { return Move(n, 0); }

std::vector<Species> build() {
    std::vector<Species> v;
    // Six-stat add: HP, ATK, DEF, SPA, SPD (special defence), SPE (speed), all as pips 1..5.
    auto add6 = [&](const char* name, int line, int stage, int hp, int atk, int def, int spa, int spd, int spe, std::array<Move, 4> m,
                    bool rare = false, bool starter = false) {
        Species s;
        s.id = static_cast<int>(v.size());
        s.name = name;
        s.line = line;
        s.stage = stage;
        s.hp = hp; s.atk = atk; s.def = def; s.spa = spa; s.spd = spd; s.spe = spe;
        s.moves = std::move(m);
        s.rare = rare;
        s.starter = starter;
        v.push_back(std::move(s));
    };
    // Original friends are written with four stats; the special pair is derived from their moves (physical or special
    // hitters) and a per-line special-bulk offset.
    static const int kSpecialBulk[27] = {0, -1, -1, -2, 2, 1, -1, 2, -1, 1, 1, 2, -2, 0, 1, 1, 1, 1, 0, -1, 1, -1, 0, 2, 1, 0, -1};
    auto add = [&](const char* name, int line, int stage, int hp, int atk, int def, int spe, std::array<Move, 4> m,
                   bool rare = false, bool starter = false) {
        double phys = 0, spec = 0;
        for (const auto& mvv : m) {
            if (!mvv.damaging()) continue;
            double p = mvv.hits.empty() ? mvv.power : 0;
            for (int h : mvv.hits) p += h;
            (mvv.special ? spec : phys) += p;
        }
        const double share = (phys + spec) > 0 ? spec / (phys + spec) : 0.5;
        int a = atk, sa = atk;
        if ((phys + spec) == 0) { a = sa = std::max(1, atk - 1); }
        else if (share >= 0.67) { a = std::max(1, atk - 2); sa = atk; }
        else if (share <= 0.33) { a = atk; sa = std::max(1, atk - 2); }
        else { a = sa = std::max(1, atk - 1); }
        const int sd = std::clamp(def + kSpecialBulk[line], 1, 5);
        add6(name, line, stage, hp, a, def, sa, sd, spe, std::move(m), rare, starter);
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
        mv("Static Nip", L).Spc().Prio(),
        util("Hyper Dash").CD(3).T(Tgt::Self).Me(EK::Haste, 3),
        mv("Jolt Rattle", L).Spc().CD(3).Fx(EK::Daze, 2),
        util("Quick Charge").CD(3).T(Tgt::Self).Prio().Me(EK::Focus)}, false, true);
    add("Zephox", 1, 2, 3, 3, 2, 5, {
        mv("Slipstream Strike", M).Special(Sp::SlipstreamBonus),
        util("Afterimage").CD(3).T(Tgt::Self).Prio().Me(EK::Evade),
        mv("Spark Chain", 0).Spc().CD(3).H({L, L}).Rand().PerHit().Fx(EK::Daze, 1, 0, 20),
        util("Windup Zip").CD(3).T(Tgt::Self).Me(EK::Haste, 2).Me(EK::Focus)});
    add("Tempestail", 1, 3, 3, 4, 2, 5, {
        mv("Thunder Lash", H).Spc().CD(2).IgnoreDef(30),
        util("Eye of the Storm").CD(4).T(Tgt::Self).Me(EK::Evade).Me(EK::Empower, 60, 1),
        mv("Rolling Thunder", 0).Spc().CD(2).H({30, 50, 70}),
        util("Gale Veil").CD(3).T(Tgt::Self).Me(EK::Haste, 3).Me(EK::Cleanse)});
    evo(3, 4, 16); evo(4, 5, 32);

    // ---- Starter line S3: Ember (burst / setup) ----
    add("Cindrel", 2, 1, 2, 4, 1, 3, {
        mv("Ember Snap", L).Spc().Fx(EK::Wound, 2, 0, 25),
        util("Kindle").CD(3).T(Tgt::Self).Me(EK::Focus),
        mv("Flare Burst", H).Spc().CD(2).Recoil(10),
        util("Smolder Aura").CD(4).Fx(EK::Wound, 3).Fx(EK::Mark)}, false, true);
    add("Ashwolf", 2, 2, 3, 4, 2, 3, {
        mv("Cinder Fang", M).Fx(EK::Wound, 2, 0, 30),
        util("Ashen Howl").CD(3).T(Tgt::Self).Me(EK::AtkMod, 30, 3),
        util("Scorched Earth").CD(4).Fx(EK::Wound, 3).Fx(EK::Slow, 2),
        mv("Backdraft", H).Spc().CD(2).Special(Sp::BackdraftBonus)});
    add("Pyrelord", 2, 3, 3, 5, 2, 3, {
        mv("Inferno Crown", H).Spc().CD(2).Fx(EK::Wound, 2),
        util("Phoenix Pact").CD(5).T(Tgt::Self).Me(EK::SelfDamage, 25).Me(EK::Empower, 80, 2),
        mv("Ember Rain", 0).Spc().CD(2).H({L, L, L}).Rand().PerHit().Fx(EK::Wound, 2, 0, 25),
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
        mv("Bubble Pop", L).Spc(),
        util("Mending Mist").CD(3).T(Tgt::Ally).Fx(EK::Heal, 20),
        util("Rinse").CD(3).T(Tgt::Ally).Fx(EK::Cleanse).Fx(EK::Regen, 2),
        util("Slick Skin").CD(3).T(Tgt::Self).Me(EK::Evade, 0, 0, 50)});
    add("Tidalisk", 4, 2, 4, 2, 3, 3, {
        mv("Crashing Wave", H).Spc().CD(2).Me(EK::Cleanse),
        util("Healing Current").CD(3).T(Tgt::Ally).Fx(EK::Heal, 35),
        mv("Undertow", M).Spc().Fx(EK::Slow, 2),
        util("Tidal Barrier").CD(4).T(Tgt::Ally).Fx(EK::Shield, 25).Fx(EK::Regen, 2)});
    evo(11, 12, 20);

    // ---- L3 debuffer ----
    add("Gloomoth", 5, 1, 2, 2, 2, 4, {
        mv("Dust Flutter", L).Spc().Fx(EK::Daze, 1),
        util("Hex Mark").CD(3).Fx(EK::Mark),
        mv("Sap Scales", M).Spc().CD(3).Drain(50),
        util("Lullaby Wings").CD(4).Fx(EK::Slow, 3)});
    add("Dusklady", 5, 2, 3, 3, 2, 4, {
        util("Doom Whisper").CD(4).Fx(EK::Doom, 3),
        util("Silken Seal").CD(3).Fx(EK::Seal),
        mv("Nightfall Wings", M).Spc().CD(3).Fx(EK::Mark).Fx(EK::Daze, 1),
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
        mv("Razor Gale", H).Spc().CD(2).Crit(25),
        util("Eye Dart").CD(3).T(Tgt::Self).Me(EK::Evade).Me(EK::Focus)});
    evo(15, 16, 20);

    // ---- L5 support ----
    add("Glimmerling", 7, 1, 3, 1, 2, 3, {
        mv("Sparkle Tap", L).Spc(),
        util("Prism Shield").CD(3).T(Tgt::Ally).Fx(EK::Shield, 15),
        util("Lucky Glint").CD(3).T(Tgt::Ally).Fx(EK::Focus).Fx(EK::NoMiss),
        util("Dazzle").CD(3).Fx(EK::Daze, 2)});
    add("Prismaw", 7, 2, 4, 2, 3, 3, {
        mv("Refract Beam", M).Spc().CD(2).IgnoreDefenses(),
        util("Crystal Aegis").CD(4).T(Tgt::Ally).Fx(EK::Shield, 30).Fx(EK::Thorns, 2),
        util("Spectrum Boost").CD(4).T(Tgt::Ally).Fx(EK::AtkMod, 20, 3).Fx(EK::DefMod, 20, 3).Fx(EK::SpeMod, 20, 3),
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
        mv("Chill Breath", L).Spc().Fx(EK::Slow, 1),
        util("Frost Veil").CD(3).T(Tgt::Self).Me(EK::Armor, 2),
        mv("Icicle Prick", M).Spc().CD(3).Fx(EK::Slow, 2, 0, 40),
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
        mv("Cinder Wing", M).Spc(),
        util("Rebirth Pyre").Once().T(Tgt::Self).Special(Sp::RebirthPyre),
        util("Ash Veil").CD(3).T(Tgt::Self).Me(EK::Shield, 15).Me(EK::Cleanse),
        mv("Solar Dive", H).Spc().CD(3).Charge(false).IgnoreDefenses()}, true);
    add("Kaleidra", 16, 1, 3, 4, 2, 3, {
        mv("Chaos Bolt", M).Spc().Special(Sp::ChaosBolt),
        util("Wild Card").CD(3).T(Tgt::Self).Special(Sp::WildCard),
        mv("Prismatic Shatter", M).Spc().CD(3).Special(Sp::PrismaticShatter),
        util("Anarchy").CD(5).T(Tgt::Self).Special(Sp::Anarchy)}, true);

    // ======================= Expansion (ids 31..47) =======================
    // ---- L9 Sporeling line (fungus: wounds and spread control) ----
    add("Sporeling", 17, 1, 3, 2, 3, 2, {
        mv("Cap Bonk", L),
        util("Spore Puff").CD(3).T(Tgt::Spread).Fx(EK::Daze, 1),
        util("Rot Touch").CD(3).Fx(EK::Wound, 3),
        util("Mulch Meal").CD(4).T(Tgt::Self).Me(EK::Heal, 20).Me(EK::Regen, 2)});
    add("Mycelord", 17, 2, 4, 3, 3, 2, {
        mv("Toadstool Slam", M).Fx(EK::Wound, 2, 0, 30),
        util("Choking Cloud").CD(4).T(Tgt::Spread).Fx(EK::Wound, 2),
        util("Fungal Network").CD(4).T(Tgt::Ally).Fx(EK::Regen, 3).Fx(EK::Shield, 10),
        util("Parasite Bloom").CD(3).Fx(EK::Wound, 3).Me(EK::Leech, 3)});
    evo(31, 32, 20);

    // ---- L10 Sandpup line (desert: evasion and slowing) ----
    add("Sandpup", 18, 1, 3, 3, 2, 3, {
        mv("Sand Kick", L).Fx(EK::Daze, 1, 0, 40),
        util("Dust Devil").CD(3).T(Tgt::Self).Me(EK::Evade).Me(EK::Haste, 1),
        mv("Quicksand Snap", M).Spc().CD(3).Fx(EK::Slow, 2),
        mv("Sinkhole Ambush", M).CD(3).Charge(true)});
    add("Dunewyrm", 18, 2, 4, 3, 3, 3, {
        mv("Sandstorm", L).Spc().CD(3).T(Tgt::Spread).Fx(EK::Daze, 2),
        util("Mirage Coil").CD(4).T(Tgt::Self).Me(EK::Evade).Me(EK::Armor, 2),
        mv("Constrict", M).CD(3).Fx(EK::Trap, 2),
        util("Serpent Dance").CD(4).T(Tgt::Ally).Fx(EK::Haste, 2).Fx(EK::Focus)});
    evo(33, 34, 20);

    // ---- L11 Cogpup line (clockwork: tempo and empowerment) ----
    add("Cogpup", 19, 1, 3, 3, 3, 3, {
        mv("Wrench Whack", L),
        util("Oil Slick").CD(3).Fx(EK::Slow, 2).Me(EK::Haste, 1),
        util("Tighten Bolts").CD(3).T(Tgt::Self).Me(EK::Armor, 2).Me(EK::Focus),
        mv("Spring Dash", M).CD(2).Prio()});
    add("Gearhound", 19, 2, 4, 3, 3, 3, {
        mv("Piston Punch", H).CD(2).IgnoreDef(20),
        util("Overclock").CD(4).T(Tgt::Self).Me(EK::AtkMod, 40, 3).Me(EK::SpeMod, 20, 3),
        mv("Gear Grind", M).Fx(EK::DefMod, -20, 2),
        util("Safety Shell").CD(4).T(Tgt::Self).Me(EK::Shield, 25).Me(EK::Cleanse)});
    evo(35, 36, 20);

    // ---- L12 Pearlclam line (shore: shells, thorns and support) ----
    add("Pearlclam", 20, 1, 4, 1, 4, 1, {
        mv("Shell Tap", L),
        util("Clam Up").CD(3).T(Tgt::Self).Me(EK::Armor, 2).Me(EK::Shield, 10),
        mv("Pearl Shot", M).Spc().CD(2).Fx(EK::Daze, 1),
        util("Tide Pool").CD(4).T(Tgt::Ally).Fx(EK::Heal, 15).Fx(EK::Regen, 2)});
    add("Pearlarch", 20, 2, 5, 2, 5, 1, {
        mv("Pearl Barrage", 0).Spc().CD(2).H({L, L, L}).Rand(),
        util("Iron Shell").CD(4).T(Tgt::Self).Me(EK::Armor, 3).Me(EK::Thorns, 3),
        mv("Riptide Slam", M).Spc().CD(3).Fx(EK::ForceSwitch),
        util("Lustrous Veil").CD(4).T(Tgt::Ally).Fx(EK::Shield, 30).Fx(EK::Cleanse)});
    evo(37, 38, 20);

    // ---- L13 Cragoat line (peaks: rams and traps) ----
    add("Cragoat", 21, 1, 3, 4, 2, 3, {
        mv("Head Butt", M).Recoil(5),
        mv("Mountain Leap", M).CD(3).Charge(true).Fx(EK::Daze, 1),
        mv("Horn Lock", M).CD(3).Fx(EK::Trap, 2),
        util("Steady Footing").CD(3).T(Tgt::Self).Me(EK::Armor, 2).Me(EK::Cleanse)});
    add("Summitaur", 21, 2, 4, 5, 3, 3, {
        mv("Avalanche Charge", H).CD(2).Recoil(8).Fx(EK::Slow, 1),
        mv("Thunderhoof", M).CD(3).T(Tgt::Spread).Fx(EK::Daze, 1),
        util("Cliffside Stance").CD(3).T(Tgt::Self).Me(EK::Counter, 25),
        util("Summit Roar").CD(4).T(Tgt::Spread).Fx(EK::Slow, 1).Me(EK::AtkMod, 25, 3)});
    evo(39, 40, 20);

    // ---- L14 Finlet line (sea: three stages, tempo into control) ----
    add("Finlet", 22, 1, 2, 2, 2, 4, {
        mv("Flick Splash", L).Spc(),
        mv("Tidal Dart", L).Spc().CD(3).Prio(),
        util("Bubble Net").CD(3).Fx(EK::Trap, 2),
        util("Schooling").CD(4).T(Tgt::Ally).Fx(EK::Haste, 2).Fx(EK::Regen, 2)});
    add("Reefray", 22, 2, 3, 3, 3, 4, {
        mv("Stingray Lash", M).Fx(EK::Wound, 2, 0, 35),
        util("Glide Away").CD(3).T(Tgt::Self).Me(EK::Evade).Me(EK::Haste, 2),
        mv("Electro Pulse", M).Spc().CD(3).T(Tgt::Spread).Fx(EK::Daze, 1),
        util("Coral Wall").CD(4).T(Tgt::Ally).Fx(EK::Armor, 3).Fx(EK::Shield, 15)});
    add("Leviathor", 22, 3, 4, 4, 3, 2, {
        mv("Abyssal Crush", H).CD(2).Fx(EK::Slow, 2),
        mv("Maelstrom", M).Spc().CD(4).T(Tgt::Spread).Fx(EK::ForceSwitch),
        util("Deep Current").CD(4).T(Tgt::Ally).Fx(EK::Regen, 4).Fx(EK::Cleanse),
        mv("Kraken's Grasp", M).CD(3).Fx(EK::Trap, 3).Fx(EK::Wound, 2)});
    evo(41, 42, 16); evo(42, 43, 32);

    // ---- Four more rares ----
    add("Eclipsa", 23, 1, 3, 3, 2, 4, {
        mv("Crescent Slash", M).Spc().Fx(EK::Mark),
        util("Lunar Eclipse").CD(5).T(Tgt::Spread).Fx(EK::Daze, 2).Fx(EK::Slow, 1),
        util("Dusk Step").CD(3).T(Tgt::Self).Me(EK::Evade).Me(EK::Haste, 1).Me(EK::PrioNext),
        util("Starlit Hex").CD(4).Fx(EK::Doom, 2).Fx(EK::Mark)}, true);
    add("Mossback", 24, 1, 5, 2, 5, 1, {
        mv("Shell Bash", M),
        util("Garden Grows").CD(4).T(Tgt::Ally).Fx(EK::Regen, 4).Fx(EK::Shield, 15),
        util("Root Snare").CD(3).Fx(EK::Trap, 2).Fx(EK::Slow, 1),
        util("Sunbaked Rest").CD(5).T(Tgt::Self).Me(EK::Heal, 40).Me(EK::Slow, 1)}, true);
    add("Hailwing", 25, 1, 3, 4, 2, 5, {
        mv("Frost Gust", L).Spc().CD(2).T(Tgt::Spread).Fx(EK::Slow, 1),
        mv("Hailstorm", 0).Spc().CD(3).H({35, 35, 35}).Rand().PerHit().Fx(EK::Slow, 1, 0, 30),
        mv("Wind Chill Dive", H).CD(3).Charge(true),
        util("Jet Stream").CD(4).T(Tgt::Ally).Fx(EK::Haste, 3).Fx(EK::Focus)}, true);
    add("Magmaroth", 26, 1, 4, 4, 3, 1, {
        mv("Magma Fist", M).Fx(EK::Wound, 2, 0, 30),
        util("Molten Carapace").CD(4).T(Tgt::Self).Me(EK::Armor, 3).Me(EK::Thorns, 3),
        mv("Eruption", M).Spc().CD(3).T(Tgt::Spread).Fx(EK::Wound, 2),
        util("Obsidian Rage").CD(5).T(Tgt::Self).Me(EK::SelfDamage, 15).Me(EK::AtkMod, 50, 3).Me(EK::Empower, 40, 1)}, true);


    // Role and one-line concept for the original 27 lines.
    static const char* kRole[27] = {"Regrowing tank", "Lightning tempo", "Burst setup", "Rock wall and counter", "Healer", "Curse and Doom", "Hit and run",
        "Crystal support", "Trapper", "Ice control", "Mirror trickster", "Time rogue", "Armored bruiser", "Vampire drainer", "Shapeshifting wall",
        "Phoenix", "Chaos prism", "Fungus control", "Desert evader", "Clockwork tempo", "Shell support", "Mountain ram", "Sea tempo", "Curse and evasion",
        "Regrowing wall", "Spread control", "Lava bruiser"};
    static const char* kBlurb[27] = {
        "A sprout-crowned tank that outlasts foes with regrowth, thorns and armor.",
        "A crackling fox that wins by acting first: priority, haste and dodging.",
        "A flame-crested hound that stacks Focus and wounds before a huge burst.",
        "A cracked-stone wall that shrugs off hits and answers with a counter.",
        "A gentle water drop that mends, cleanses and slips away from danger.",
        "A patterned moth that marks, slows and eventually dooms its prey.",
        "A quick bird that pecks then swaps out, diving from the sky with a dodging charge.",
        "A faceted crystal that shields its allies and dazzles foes.",
        "A claw-handed mole that digs in, dodges underground and traps foes.",
        "A drifting ice spirit that slows, armors up and freezes foes in place.",
        "A mirror-bellied rat that copies moves, reflects attacks and steals buffs.",
        "An owl that rewinds cooldowns, seeds Doom and slows time itself.",
        "An armored beetle that stacks plates and crushes with cannonball rolls.",
        "A bat that drains life, wounds foes and trades its own health for power.",
        "A jelly cube that absorbs hits, copies stat changes and slams back what it took.",
        "A flame-winged phoenix that revives once and dives for huge damage.",
        "A rainbow prism that strips buffs, wipes the field and gambles on random effects.",
        "A spotted mushroom that spreads spore clouds, wounds and regrowth.",
        "A dune pup that kicks sand, slows foes and ambushes from below.",
        "A gear-driven hound that speeds itself up and shatters defenses.",
        "A pearl-bearing clam that armors up and heals its allies.",
        "A sure-footed goat that charges, locks horns and traps foes.",
        "A darting fish that grows into a tide-controlling sea beast.",
        "A moon spirit that hexes foes and vanishes before they can strike.",
        "A turtle with a living garden on its shell that heals the whole team.",
        "An ice-winged bird that chills the whole field.",
        "A lava-veined beast that wounds everything around it."};
    for (auto& sp : v) if (sp.line < 27) { sp.role = kRole[sp.line]; sp.blurb = kBlurb[sp.line]; }

    // Habitats: where each line can be found (bit 0 Meadow, 1 Woods, 2 Caverns, 3 Shore, 4 Peaks, 5 Ruins).
    const int hab[27] = {3, 17, 33, 20, 9, 34, 17, 36, 5, 16, 40, 32, 4, 6, 40, 48, 32, 2, 5, 36, 8, 16, 8, 34, 10, 16, 36};
    for (auto& sp : v) if (sp.line < 27) sp.habitats = hab[sp.line];

    // ======================= Expansion II (ids 48..149), generated by tools/gen_roster.py =======================
#include "roster_ext.inc"

    // No two friends share the exact same six-stat spread: a duplicate is nudged by moving one pip between two stats.
    {
        std::set<std::array<int, 6>> seen;
        for (auto& sp : v) {
            int* st[6] = {&sp.hp, &sp.atk, &sp.def, &sp.spa, &sp.spd, &sp.spe};
            auto key = [&]() { return std::array<int, 6>{*st[0], *st[1], *st[2], *st[3], *st[4], *st[5]}; };
            for (int attempt = 0; seen.count(key()) && attempt < 5000; ++attempt) {
                const int up = (sp.id * 5 + attempt * 7) % 6, down = (sp.id * 3 + attempt * 11 + 1) % 6;
                if (up != down && *st[up] < 5 && *st[down] > 1) { ++*st[up]; --*st[down]; }
            }
            seen.insert(key());
        }
    }
    return v;
}

}  // namespace

const std::vector<Species>& allSpecies() {
    static const std::vector<Species> s = build();
    return s;
}

const std::vector<Area>& allAreas() {
    static const std::vector<Area> a = {
        {0, "Sunny Meadow", "Gentle fields where most friends start out.", 1, 0, 1.0, 0, 0x6EC27A, 0x2E7D4F},
        {1, "Whispering Woods", "Damp, shady trees full of fungus and moths.", 2, 1, 1.2, 3, 0x3F8F5A, 0x16402C},
        {2, "Echo Caverns", "Dark tunnels with crystals, claws and metal.", 4, 2, 1.4, 8, 0x6A6F86, 0x23263A},
        {3, "Sapphire Shore", "Tidepools and reefs, calm until it isn't.", 8, 3, 1.6, 14, 0x58B4D8, 0x1E5E8C},
        {4, "Stormcrest Peaks", "Freezing winds and sure-footed climbers.", 16, 4, 1.8, 22, 0xA9C6E8, 0x4A5F86},
        {5, "Ember Ruins", "Ancient halls where time and fire misbehave.", 32, 6, 2.2, 32, 0xC8684A, 0x4A2230}};
    return a;
}

const Area& area(int id) { return allAreas().at(static_cast<size_t>(id)); }

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
    st.spa = stat(s.spa);
    st.spd = stat(s.spd);
    st.spe = stat(s.spe);
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
