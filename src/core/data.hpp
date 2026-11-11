// Static game content: moves and the 31 friends. See ROSTER.md for the design.
#pragma once
#include <array>
#include <string>
#include <vector>

namespace gf {

// ---- Moves ----------------------------------------------------------------

enum class Tgt { Enemy, Spread, Self, Ally };

// Effect kinds. For statuses, `a` is the duration in turns unless noted.
enum class EK {
    Wound, Daze, Slow, Haste, Armor, Thorns, Regen, Focus, Mark, Trap, Stun, Taunt, Doom, Seal,
    Shield,        // a = % of max HP
    Heal,          // a = % of max HP
    Cleanse,
    ForceSwitch,
    AtkMod,        // a = percent (negative for debuff), b = turns
    DefMod,
    SpdMod,
    Evade,         // chance = probability (default 100)
    Counter,       // a = damage multiplier in tenths (15 = 1.5x of damage taken)
    CounterHit,    // a = power of retaliation, b = wound turns on attacker
    CounterAbsorb, // negate the hit and heal 50% of it
    Empower,       // a = percent bonus, b = number of attacks
    NoMiss,
    PrioNext,      // the user's next move gets Priority
    SlowAttackers, // a = turns; attackers get Slow 1
    ReflectDebuff,
    ReflectDamage,
    SelfDamage,    // a = % of max HP lost (cannot kill)
    Plate,
    Leech          // a = turns; user heals 5% max HP each turn while target is wounded
};

// Special behaviours that need custom code in the battle engine.
enum class Sp {
    None, SlipstreamBonus, WardenBonus, BackdraftBonus, CannonballBonus, ShapeshiftSlam, Mimic,
    InvertBuffs, StealBuff, CopyMods, ResetCooldowns, WildCard, ChaosBolt, PrismaticShatter, Anarchy,
    RebirthPyre
};

struct Effect {
    EK k;
    int a = 0, b = 0;
    int chance = 100;
    bool self = false;  // applies to the user instead of the target
};

struct Move {
    std::string name;
    int power = 0;  // 0 = utility
    int cd = 0;
    Tgt tgt = Tgt::Enemy;
    std::vector<int> hits;     // multi-hit powers (overrides power)
    bool randomTargets = false;
    bool perHitFx = false;     // roll effect chances per hit
    int prio = 0;
    bool pivot = false;
    bool charge = false;
    bool chargeEvade = false;
    bool once = false;
    int ignoreDef = 0;         // % of target DEF ignored
    bool ignoreDefenses = false;  // ignores Shield and Armor
    int crit = 0;              // % chance of 1.5x
    int recoil = 0;            // % of max HP lost by user when attacking
    int drain = 0;             // % of damage dealt healed
    int mimicPct = 0;
    Sp sp = Sp::None;
    std::vector<Effect> fx;

    Move() = default;
    Move(const char* n, int p) : name(n), power(p) {}
    Move& CD(int v) { cd = v; return *this; }
    Move& T(Tgt t) { tgt = t; return *this; }
    Move& H(std::vector<int> h) { hits = std::move(h); return *this; }
    Move& Rand() { randomTargets = true; return *this; }
    Move& PerHit() { perHitFx = true; return *this; }
    Move& Prio() { prio = 1; return *this; }
    Move& Pivot() { pivot = true; return *this; }
    Move& Charge(bool evade) { charge = true; chargeEvade = evade; return *this; }
    Move& Once() { once = true; return *this; }
    Move& IgnoreDef(int v) { ignoreDef = v; return *this; }
    Move& IgnoreDefenses() { ignoreDefenses = true; return *this; }
    Move& Crit(int v) { crit = v; return *this; }
    Move& Recoil(int v) { recoil = v; return *this; }
    Move& Drain(int v) { drain = v; return *this; }
    Move& Mimic(int pct) { sp = Sp::Mimic; mimicPct = pct; return *this; }
    Move& Special(Sp s) { sp = s; return *this; }
    Move& Fx(EK k, int a = 0, int b = 0, int chance = 100) { fx.push_back({k, a, b, chance, false}); return *this; }
    Move& Me(EK k, int a = 0, int b = 0, int chance = 100) { fx.push_back({k, a, b, chance, true}); return *this; }
    bool damaging() const { return power > 0 || !hits.empty(); }
    bool multiHit() const { return hits.size() > 1; }
};

// ---- Species ---------------------------------------------------------------

struct Species {
    int id = 0;
    std::string name;
    int line = 0;        // line index, for colouring sprites
    int stage = 1;       // 1..3
    int evolvesTo = -1;  // species id
    int evolveLevel = 0;
    bool rare = false;
    bool starter = false;
    int hp = 3, atk = 3, def = 3, spd = 3;  // pips 1..5
    std::array<Move, 4> moves;
};

struct Stats { int hp, atk, def, spd; };

constexpr int kSpeciesCount = 31;
constexpr int kMaxLevel = 50;
constexpr int kPartySize = 6;
constexpr int kActiveSlots = 3;

const std::vector<Species>& allSpecies();
const Species& species(int id);
Stats statsFor(const Species& s, int level);
int xpToNext(int level);
// Species with no pre-evolution (stage 1 or rare).
std::vector<int> baseSpecies();
std::vector<int> starterSpecies();

}  // namespace gf
