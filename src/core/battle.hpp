// Turn-based triple-battle engine. Pure logic: no I/O, no graphics, deterministic given a seed.
#pragma once
#include <array>
#include <cstdint>
#include <random>
#include <string>
#include <vector>

#include "data.hpp"

namespace gf {

struct Ref {
    int side = 0;  // 0 = player, 1 = enemy
    int slot = 0;  // 0..2
};

struct Fighter {
    int speciesId = 0;
    int level = 1;
    int maxHp = 1, hp = 1, atk = 1, def = 1, spd = 1;
    bool sleepy = false;
    int friendId = -1;  // index in the player's roster, or -1 for foes

    int cd[4] = {0, 0, 0, 0};
    bool used[4] = {false, false, false, false};
    int lastMove = -1;
    int lastDamageTaken = 0;

    int wound = 0, daze = 0, slow = 0, haste = 0, armor = 0, thorns = 0, regen = 0;
    int trap = 0, stun = 0, taunt = 0, doom = 0, seal = 0, sealMove = -1;
    bool focus = false, mark = false, noMiss = false, prioNext = false;
    bool evade = false;
    int evadeChance = 100;
    int shield = 0;
    int atkMod = 0, atkModT = 0, defMod = 0, defModT = 0, spdMod = 0, spdModT = 0;
    int empowerPct = 0, empowerN = 0;
    int slowAttackers = 0;
    bool reflectDebuff = false, reflectDamage = false;
    int counterMult = 0, counterHitPower = 0, counterWound = 0;
    bool counterAbsorb = false;
    int plates = 0;
    bool rebirth = false;
    int leechTurns = 0, leechSrcSide = -1, leechSrcIdx = -1;
    bool charging = false;
    int chargeMove = -1;
    Ref chargeTarget;
    bool actedThisTurn = false;

    bool alive() const { return hp > 0; }
    double hpPct() const { return maxHp > 0 ? static_cast<double>(hp) / maxHp : 0.0; }
    const Species& sp() const { return species(speciesId); }
    void clearStatuses();  // keeps HP and cooldowns
};

Fighter makeFighter(int speciesId, int level, bool sleepy = false, int friendId = -1);

struct Action {
    enum Type { None, UseMove, Switch } type = None;
    int move = 0;
    Ref target;
    int bench = -1;  // team index for Switch
};

struct Side {
    std::vector<Fighter> team;
    std::array<int, kActiveSlots> active{-1, -1, -1};  // team indices
    bool isPlayer = false;
};

// A frozen copy of the battlefield, used to replay a turn step by step.
struct Snapshot {
    std::array<std::vector<Fighter>, 2> team;
    std::array<std::array<int, kActiveSlots>, 2> active{};
};

struct Event {
    std::string text;
    Ref actor{-1, -1};  // who is acting, when this line announces a move
    Snapshot snap;      // state right after this event
};

class Battle {
public:
    Battle(std::vector<Fighter> player, std::vector<Fighter> enemy, uint32_t seed, bool wild = false, bool smartAi = false);

    std::array<Side, 2> sides;
    std::vector<std::string> log;
    int turn = 0;
    int winner = -1;  // 0 = player, 1 = enemy
    bool wild = false;
    bool smartAi = false;           // bosses: no randomness in move choice
    bool autoReplacePlayer = false; // simulations: pick bench replacements automatically
    bool recordEvents = false;      // UI: keep a snapshot after every log line so a turn can be replayed
    std::vector<Event> events;
    int areaId = 0;
    Snapshot snapshot() const;

    // Accessors
    bool slotFilled(int side, int slot) const { return sides[side].active[slot] >= 0; }
    Fighter& fighter(int side, int slot) { return sides[side].team[static_cast<size_t>(sides[side].active[slot])]; }
    const Fighter& fighter(int side, int slot) const { return sides[side].team[static_cast<size_t>(sides[side].active[slot])]; }
    std::vector<int> benchAlive(int side) const;  // team indices, alive and not active
    bool over() const { return winner >= 0; }
    std::string nameOf(int side, int slot) const;

    // Player helpers
    bool moveUsable(int side, int slot, int moveIdx) const;
    std::vector<Ref> validTargets(int side, int slot, int moveIdx) const;  // empty = no target needed
    bool needsTarget(int side, int slot, int moveIdx) const;
    bool canSwitch(int side, int slot) const;

    // AI
    Action aiChoose(int side, int slot);

    // Resolve one turn. Player actions are indexed by slot; enemy actions come from the AI.
    void resolveTurn(const std::array<Action, 3>& playerActions);
    // Fill every empty player slot from the bench, in order, using a player's own choice.
    std::vector<int> emptyPlayerSlotsNeedingReplacement() const;
    void replace(int side, int slot, int benchIdx);
    void autoReplace(int side);

    // Catch a wild foe (used by the Catch Charm). On success the foe leaves the battle and is recorded in `caught`.
    struct Caught { int species; int level; };
    bool tryCatch(int slot, double chance);
    std::vector<Caught> caught;

    int rnd(int n);  // uniform in [0, n)

private:
    std::mt19937 rng_;
    struct Entry { Ref who; Action act; int prio; double spd; uint32_t tie; int teamIdx; };

    double effAtk(const Fighter& f) const;
    double effDef(const Fighter& f) const;
    double effSpd(const Fighter& f) const;
    double estimateDamage(const Fighter& a, const Fighter& d, double power) const;

    void execute(const Entry& e);
    void useMove(Ref user, const Move& m, int moveIdx, Ref primary, double scale);
    int dealHit(Ref u, Ref t, double power, const Move& m, double mult, int fixedDamage, bool& focusUsed);
    void applyEffect(const Effect& e, Ref target, Ref source);
    void hurt(Ref t, int amount, bool bypassShield);
    void heal(Ref t, int amount);
    void faintCheck(Ref t);
    void doSwitch(Ref who, int benchIdx);
    bool forceSwitch(Ref t);
    void pivot(Ref who);
    void endOfTurn();
    void checkWinner();
    void say(const std::string& text, Ref actor = Ref{-1, -1});
    int countBuffs(const Fighter& f) const;
    void clearBuffs(Fighter& f);
    Ref randomEnemy(int side);
    std::vector<Ref> livingEnemies(int side) const;
    std::vector<Ref> livingAllies(int side) const;
    double scoreMove(int side, int slot, int moveIdx, Ref& bestTarget);
    Fighter& at(Ref r) { return fighter(r.side, r.slot); }
    const Fighter& at(Ref r) const { return fighter(r.side, r.slot); }
};

}  // namespace gf
