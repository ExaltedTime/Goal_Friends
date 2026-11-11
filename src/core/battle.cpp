#include "battle.hpp"

#include <algorithm>
#include <cmath>

namespace gf {

namespace {
const Move& fallbackMove() {
    static const Move m = Move("Scuffle", 40);
    return m;
}

bool isDebuff(const Effect& e) {
    switch (e.k) {
        case EK::Wound: case EK::Daze: case EK::Slow: case EK::Trap: case EK::Stun: case EK::Taunt:
        case EK::Doom: case EK::Seal: case EK::Mark: case EK::ForceSwitch:
            return true;
        case EK::AtkMod: case EK::DefMod: case EK::SpdMod:
            return e.a < 0;
        default:
            return false;
    }
}
}  // namespace

// ---------------------------------------------------------------------------
// Fighter
// ---------------------------------------------------------------------------

void Fighter::clearStatuses() {
    wound = daze = slow = haste = armor = thorns = regen = 0;
    trap = stun = taunt = doom = seal = 0;
    sealMove = -1;
    focus = mark = noMiss = prioNext = evade = false;
    evadeChance = 100;
    shield = 0;
    atkMod = atkModT = defMod = defModT = spdMod = spdModT = 0;
    empowerPct = empowerN = 0;
    slowAttackers = 0;
    reflectDebuff = reflectDamage = false;
    counterMult = counterHitPower = counterWound = 0;
    counterAbsorb = false;
    plates = 0;
    leechTurns = 0;
    leechSrcSide = leechSrcIdx = -1;
    charging = false;
    chargeMove = -1;
    actedThisTurn = false;
}

Fighter makeFighter(int speciesId, int level, bool sleepy, int friendId) {
    Fighter f;
    f.speciesId = speciesId;
    f.level = level;
    const Stats s = statsFor(species(speciesId), level);
    f.maxHp = f.hp = s.hp;
    f.atk = s.atk;
    f.def = s.def;
    f.spd = s.spd;
    f.sleepy = sleepy;
    f.friendId = friendId;
    return f;
}

// ---------------------------------------------------------------------------
// Setup and accessors
// ---------------------------------------------------------------------------

Battle::Battle(std::vector<Fighter> player, std::vector<Fighter> enemy, uint32_t seed, bool wild_, bool smart)
    : wild(wild_), smartAi(smart), rng_(seed) {
    sides[0].team = std::move(player);
    sides[0].isPlayer = true;
    sides[1].team = std::move(enemy);
    for (auto& side : sides) {
        int slot = 0;
        for (size_t i = 0; i < side.team.size() && slot < kActiveSlots; ++i)
            if (side.team[i].alive()) side.active[static_cast<size_t>(slot++)] = static_cast<int>(i);
    }
    log.push_back(wild ? "A wild encounter!" : "Battle start!");
    checkWinner();
}

int Battle::rnd(int n) {
    if (n <= 1) return 0;
    return static_cast<int>(rng_() % static_cast<uint32_t>(n));
}

std::vector<int> Battle::benchAlive(int side) const {
    std::vector<int> out;
    const Side& s = sides[side];
    for (size_t i = 0; i < s.team.size(); ++i) {
        if (!s.team[i].alive()) continue;
        bool active = false;
        for (int a : s.active)
            if (a == static_cast<int>(i)) active = true;
        if (!active) out.push_back(static_cast<int>(i));
    }
    return out;
}

std::string Battle::nameOf(int side, int slot) const {
    const Fighter& f = fighter(side, slot);
    return (side == 1 ? "Foe " : "") + f.sp().name;
}

double Battle::effAtk(const Fighter& f) const {
    double v = f.atk;
    if (f.atkModT > 0) v *= 1.0 + f.atkMod / 100.0;
    if (f.sleepy) v *= 0.8;
    return std::max(1.0, v);
}
double Battle::effDef(const Fighter& f) const {
    double v = f.def;
    if (f.defModT > 0) v *= 1.0 + f.defMod / 100.0;
    return std::max(1.0, v);
}
double Battle::effSpd(const Fighter& f) const {
    double v = f.spd;
    if (f.slow > 0) v *= 0.7;
    if (f.haste > 0) v *= 1.4;
    if (f.spdModT > 0) v *= 1.0 + f.spdMod / 100.0;
    if (f.sleepy) v *= 0.8;
    return std::max(1.0, v);
}

double Battle::estimateDamage(const Fighter& a, const Fighter& d, double power) const {
    return ((2.0 * a.level / 5 + 2) * power * effAtk(a) / effDef(d) / 50 + 2) * 0.95;
}

std::vector<Ref> Battle::livingEnemies(int side) const {
    std::vector<Ref> out;
    const int o = 1 - side;
    for (int s = 0; s < kActiveSlots; ++s)
        if (slotFilled(o, s) && fighter(o, s).alive()) out.push_back({o, s});
    return out;
}
std::vector<Ref> Battle::livingAllies(int side) const {
    std::vector<Ref> out;
    for (int s = 0; s < kActiveSlots; ++s)
        if (slotFilled(side, s) && fighter(side, s).alive()) out.push_back({side, s});
    return out;
}
Ref Battle::randomEnemy(int side) {
    auto e = livingEnemies(side);
    if (e.empty()) return {1 - side, 0};
    return e[static_cast<size_t>(rnd(static_cast<int>(e.size())))];
}

bool Battle::moveUsable(int side, int slot, int mi) const {
    if (!slotFilled(side, slot) || mi < 0 || mi > 3) return false;
    const Fighter& f = fighter(side, slot);
    const Move& m = f.sp().moves[static_cast<size_t>(mi)];
    if (f.cd[mi] > 0) return false;
    if (m.once && f.used[mi]) return false;
    if (f.seal > 0 && f.sealMove == mi) return false;
    if (f.taunt > 0 && !m.damaging()) return false;
    return true;
}

bool Battle::needsTarget(int side, int slot, int mi) const {
    if (mi < 0 || mi > 3) return true;  // fallback move targets one enemy
    const Move& m = fighter(side, slot).sp().moves[static_cast<size_t>(mi)];
    return (m.tgt == Tgt::Enemy && !m.randomTargets) || m.tgt == Tgt::Ally;
}

std::vector<Ref> Battle::validTargets(int side, int slot, int mi) const {
    if (!needsTarget(side, slot, mi)) return {};
    if (mi >= 0 && mi <= 3 && fighter(side, slot).sp().moves[static_cast<size_t>(mi)].tgt == Tgt::Ally)
        return livingAllies(side);
    return livingEnemies(side);
}

bool Battle::canSwitch(int side, int slot) const {
    return slotFilled(side, slot) && fighter(side, slot).trap == 0 && !benchAlive(side).empty();
}

int Battle::countBuffs(const Fighter& f) const {
    int n = 0;
    n += f.haste > 0; n += f.armor > 0; n += f.regen > 0; n += f.thorns > 0; n += f.shield > 0;
    n += f.focus; n += f.atkModT > 0 && f.atkMod > 0; n += f.defModT > 0 && f.defMod > 0;
    n += f.spdModT > 0 && f.spdMod > 0; n += f.empowerN > 0;
    return n;
}
void Battle::clearBuffs(Fighter& f) {
    f.haste = f.armor = f.regen = f.thorns = 0;
    f.shield = 0;
    f.focus = false;
    if (f.atkMod > 0) f.atkModT = 0;
    if (f.defMod > 0) f.defModT = 0;
    if (f.spdMod > 0) f.spdModT = 0;
    f.empowerN = f.empowerPct = 0;
}

// ---------------------------------------------------------------------------
// Damage and effects
// ---------------------------------------------------------------------------

void Battle::faintCheck(Ref t) {
    Fighter& f = at(t);
    if (f.hp > 0) return;
    if (f.rebirth) {
        f.rebirth = false;
        f.hp = std::max(1, f.maxHp * 40 / 100);
        log.push_back(nameOf(t.side, t.slot) + " rises from the ashes!");
        return;
    }
    f.hp = 0;
    log.push_back(nameOf(t.side, t.slot) + " fainted!");
}

void Battle::hurt(Ref t, int amount, bool bypassShield) {
    Fighter& f = at(t);
    if (!f.alive() || amount <= 0) return;
    f.lastDamageTaken = amount;
    if (!bypassShield && f.shield > 0) {
        const int ab = std::min(f.shield, amount);
        f.shield -= ab;
        amount -= ab;
    }
    f.hp = std::max(0, f.hp - amount);
    faintCheck(t);
}

void Battle::heal(Ref t, int amount) {
    Fighter& f = at(t);
    if (!f.alive() || amount <= 0) return;
    f.hp = std::min(f.maxHp, f.hp + amount);
}

int Battle::dealHit(Ref u, Ref t, double power, const Move& m, double mult, int fixed, bool& focusUsed) {
    Fighter& a = at(u);
    Fighter& d = at(t);
    if (!d.alive()) return -1;
    if (d.evade) {
        d.evade = false;
        if (rnd(100) < d.evadeChance) {
            log.push_back(nameOf(t.side, t.slot) + " evades!");
            return -1;
        }
    }
    if (a.daze > 0 && !a.noMiss && rnd(100) < 30) {
        log.push_back(nameOf(u.side, u.slot) + "'s attack misses!");
        return -1;
    }
    double dmg;
    if (fixed > 0) {
        dmg = fixed;
    } else {
        const double atk = effAtk(a);
        const double def = effDef(d) * (1.0 - m.ignoreDef / 100.0);
        dmg = ((2.0 * a.level / 5 + 2) * power * atk / std::max(1.0, def) / 50 + 2) * (0.9 + 0.1 * (rnd(1001) / 1000.0));
    }
    dmg *= mult;
    if (a.focus) { dmg *= 1.5; focusUsed = true; }
    if (a.empowerN > 0) { dmg *= 1.0 + a.empowerPct / 100.0; focusUsed = true; }
    if (d.mark) { dmg *= 1.5; d.mark = false; }
    if (!m.ignoreDefenses && d.armor > 0) dmg *= 0.7;
    dmg *= 1.0 - 0.06 * d.plates;
    if (m.crit > 0 && rnd(100) < m.crit) {
        dmg *= 1.5;
        log.push_back("A critical hit!");
    }
    const int dm = std::max(1, static_cast<int>(dmg));

    if (d.reflectDamage) {
        d.reflectDamage = false;
        log.push_back(nameOf(t.side, t.slot) + " reflects the attack!");
        hurt(u, dm, false);
        return 0;
    }
    if (d.counterAbsorb) {
        d.counterAbsorb = false;
        heal(t, dm / 2);
        log.push_back(nameOf(t.side, t.slot) + " absorbs the hit!");
        return 0;
    }

    hurt(t, dm, m.ignoreDefenses);
    log.push_back(nameOf(t.side, t.slot) + " takes " + std::to_string(dm) + " damage.");

    if (d.alive()) {
        if (d.counterMult > 0) {
            const int r = std::max(1, dm * d.counterMult / 10);
            d.counterMult = 0;
            log.push_back(nameOf(t.side, t.slot) + " strikes back for " + std::to_string(r) + "!");
            hurt(u, r, false);
        }
        if (d.counterHitPower > 0) {
            const int r = std::max(1, static_cast<int>(estimateDamage(d, a, d.counterHitPower)));
            const int w = d.counterWound;
            d.counterHitPower = d.counterWound = 0;
            log.push_back(nameOf(t.side, t.slot) + " retaliates for " + std::to_string(r) + "!");
            hurt(u, r, false);
            if (a.alive() && w > 0) a.wound = std::max(a.wound, w);
        }
    }
    if (d.thorns > 0 && a.alive()) hurt(u, std::max(1, dm * 25 / 100), false);
    if (d.slowAttackers > 0) a.slow = std::max(a.slow, 1);
    if (m.drain > 0) heal(u, dm * m.drain / 100);
    return dm;
}

void Battle::applyEffect(const Effect& e, Ref target, Ref source) {
    if (e.k != EK::Evade && e.chance < 100 && rnd(100) >= e.chance) return;
    Fighter* t = &at(target);
    if (!t->alive() && e.k != EK::Heal) return;

    if (isDebuff(e) && target.side != source.side && t->reflectDebuff) {
        t->reflectDebuff = false;
        log.push_back(nameOf(target.side, target.slot) + " reflects the effect!");
        target = source;
        t = &at(target);
    }
    const std::string who = nameOf(target.side, target.slot);
    switch (e.k) {
        case EK::Wound: t->wound = std::max(t->wound, e.a); log.push_back(who + " is wounded."); break;
        case EK::Daze: t->daze = std::max(t->daze, e.a); log.push_back(who + " is dazed."); break;
        case EK::Slow: t->slow = std::max(t->slow, e.a); log.push_back(who + " is slowed."); break;
        case EK::Haste: t->haste = std::max(t->haste, e.a); log.push_back(who + " speeds up!"); break;
        case EK::Armor: t->armor = std::max(t->armor, e.a); log.push_back(who + " is armored."); break;
        case EK::Thorns: t->thorns = std::max(t->thorns, e.a); log.push_back(who + " grows thorns."); break;
        case EK::Regen: t->regen = std::max(t->regen, e.a); log.push_back(who + " starts regenerating."); break;
        case EK::Focus: t->focus = true; log.push_back(who + " focuses."); break;
        case EK::Mark: t->mark = true; log.push_back(who + " is marked."); break;
        case EK::Trap: t->trap = std::max(t->trap, e.a); log.push_back(who + " is trapped."); break;
        case EK::Stun: t->stun = 1; log.push_back(who + " is stunned!"); break;
        case EK::Taunt: t->taunt = std::max(t->taunt, e.a); break;
        case EK::Doom: t->doom = e.a; log.push_back(who + " is doomed."); break;
        case EK::Seal:
            if (t->lastMove >= 0) {
                t->sealMove = t->lastMove;
                t->seal = 2;
                log.push_back(who + "'s last move is sealed.");
            }
            break;
        case EK::Shield:
            t->shield = std::max(t->shield, t->maxHp * e.a / 100);
            log.push_back(who + " gains a shield.");
            break;
        case EK::Heal:
            heal(target, t->maxHp * e.a / 100);
            log.push_back(who + " recovers HP.");
            break;
        case EK::Cleanse:
            t->wound = t->daze = t->slow = t->trap = t->stun = t->taunt = t->doom = t->seal = 0;
            t->mark = false;
            if (t->atkMod < 0) t->atkModT = 0;
            if (t->defMod < 0) t->defModT = 0;
            if (t->spdMod < 0) t->spdModT = 0;
            break;
        case EK::ForceSwitch:
            forceSwitch(target);
            break;
        case EK::AtkMod: t->atkMod = e.a; t->atkModT = e.b; break;
        case EK::DefMod: t->defMod = e.a; t->defModT = e.b; break;
        case EK::SpdMod: t->spdMod = e.a; t->spdModT = e.b; break;
        case EK::Evade: t->evade = true; t->evadeChance = e.chance; break;
        case EK::Counter: t->counterMult = e.a; break;
        case EK::CounterHit: t->counterHitPower = e.a; t->counterWound = e.b; break;
        case EK::CounterAbsorb: t->counterAbsorb = true; break;
        case EK::Empower: t->empowerPct = e.a; t->empowerN = e.b; break;
        case EK::NoMiss: t->noMiss = true; break;
        case EK::PrioNext: t->prioNext = true; break;
        case EK::SlowAttackers: t->slowAttackers = e.a; break;
        case EK::ReflectDebuff: t->reflectDebuff = true; break;
        case EK::ReflectDamage: t->reflectDamage = true; break;
        case EK::SelfDamage:
            t->hp = std::max(1, t->hp - std::max(1, t->maxHp * e.a / 100));
            break;
        case EK::Plate: t->plates = std::min(5, t->plates + 1); log.push_back(who + " adds a plate."); break;
        case EK::Leech:
            t->leechTurns = e.a;
            t->leechSrcSide = source.side;
            t->leechSrcIdx = sides[source.side].active[static_cast<size_t>(source.slot)];
            break;
    }
}

// ---------------------------------------------------------------------------
// Switching
// ---------------------------------------------------------------------------

void Battle::doSwitch(Ref who, int benchIdx) {
    Side& s = sides[who.side];
    if (benchIdx < 0 || static_cast<size_t>(benchIdx) >= s.team.size() || !s.team[static_cast<size_t>(benchIdx)].alive()) return;
    for (int a : s.active)
        if (a == benchIdx) return;
    Fighter& old = at(who);
    if (old.alive() && old.trap > 0) {
        log.push_back(nameOf(who.side, who.slot) + " is trapped and can't leave!");
        return;
    }
    const std::string oldName = nameOf(who.side, who.slot);
    old.clearStatuses();
    s.active[static_cast<size_t>(who.slot)] = benchIdx;
    log.push_back(oldName + " swaps out for " + nameOf(who.side, who.slot) + ".");
}

bool Battle::forceSwitch(Ref t) {
    if (at(t).trap > 0) {
        log.push_back(nameOf(t.side, t.slot) + " can't be moved!");
        return false;
    }
    auto bench = benchAlive(t.side);
    if (bench.empty()) {
        log.push_back("But there was nobody to swap in.");
        return false;
    }
    const int pick = bench[static_cast<size_t>(rnd(static_cast<int>(bench.size())))];
    doSwitch(t, pick);
    return true;
}

void Battle::pivot(Ref who) {
    auto bench = benchAlive(who.side);
    if (bench.empty() || at(who).trap > 0) return;
    int best = bench[0];
    for (int b : bench)
        if (sides[who.side].team[static_cast<size_t>(b)].hpPct() > sides[who.side].team[static_cast<size_t>(best)].hpPct()) best = b;
    doSwitch(who, best);
}

// ---------------------------------------------------------------------------
// Using moves
// ---------------------------------------------------------------------------

void Battle::useMove(Ref user, const Move& m, int /*moveIdx*/, Ref primary, double scale) {
    Fighter& f = at(user);
    const bool hadNoMiss = f.noMiss;
    log.push_back(nameOf(user.side, user.slot) + " uses " + m.name + "!");
    if ((m.tgt == Tgt::Enemy || m.tgt == Tgt::Spread) && livingEnemies(user.side).empty()) return;  // nobody left to target

    // Mimic: borrow the target's last-used move.
    if (m.sp == Sp::Mimic) {
        Ref t = primary;
        if (t.side == user.side || !slotFilled(t.side, t.slot) || !at(t).alive()) t = randomEnemy(user.side);
        const Fighter& tf = at(t);
        if (tf.lastMove < 0 || tf.lastMove > 3 || tf.sp().moves[static_cast<size_t>(tf.lastMove)].sp == Sp::Mimic) {
            log.push_back("But it failed.");
            return;
        }
        Move c = tf.sp().moves[static_cast<size_t>(tf.lastMove)];
        c.charge = false; c.once = false; c.pivot = false; c.cd = 0;
        useMove(user, c, -1, t, m.mimicPct / 100.0);
        return;
    }

    // Targets
    std::vector<Ref> targets;
    switch (m.tgt) {
        case Tgt::Self: targets = {user}; break;
        case Tgt::Ally: {
            Ref t = primary;
            if (t.side != user.side || !slotFilled(t.side, t.slot) || !at(t).alive()) t = user;
            targets = {t};
            break;
        }
        case Tgt::Spread: targets = livingEnemies(user.side); break;
        case Tgt::Enemy:
            if (!m.randomTargets) {
                Ref t = primary;
                if (t.side == user.side || !slotFilled(t.side, t.slot) || !at(t).alive()) t = randomEnemy(user.side);
                targets = {t};
            }
            break;
    }
    const bool damaging = m.damaging() || m.sp == Sp::ShapeshiftSlam;
    if (damaging && livingEnemies(user.side).empty()) return;

    bool anyHit = false;

    if (damaging) {
        // Build the (target, power) list for this move.
        std::vector<std::pair<Ref, double>> shots;
        std::vector<int> powers = m.hits.empty() ? std::vector<int>{m.power} : m.hits;
        if (m.sp == Sp::ShapeshiftSlam) powers = {70};
        if (m.randomTargets) {
            for (int p : powers) shots.push_back({randomEnemy(user.side), static_cast<double>(p)});
        } else if (m.tgt == Tgt::Spread) {
            for (const Ref& t : targets) shots.push_back({t, static_cast<double>(powers[0])});
        } else {
            for (int p : powers) shots.push_back({targets[0], static_cast<double>(p)});
        }

        if (m.sp == Sp::PrismaticShatter && !shots.empty()) {
            // handled per-target below
        }
        bool focusUsed = false;
        std::vector<Ref> landed;
        for (auto& shot : shots) {
            Ref t = shot.first;
            if (!slotFilled(t.side, t.slot) || !at(t).alive()) {
                if (livingEnemies(user.side).empty()) break;
                t = randomEnemy(user.side);
            }
            double mult = scale;
            if (m.tgt == Tgt::Spread) mult *= 0.75;
            int fixed = 0;
            switch (m.sp) {
                case Sp::SlipstreamBonus: if (!at(t).actedThisTurn) mult *= 1.3; break;
                case Sp::WardenBonus: if (f.armor > 0 || f.shield > 0 || f.thorns > 0) mult *= 1.5; break;
                case Sp::BackdraftBonus: if (at(t).wound > 0) mult *= 1.5; break;
                case Sp::CannonballBonus: mult *= 1.0 + 0.1 * f.plates; break;
                case Sp::ShapeshiftSlam: if (f.lastDamageTaken > 0) fixed = std::max(1, static_cast<int>(f.lastDamageTaken * 1.2)); break;
                case Sp::PrismaticShatter: {
                    const int n = countBuffs(at(t));
                    if (n > 0) {
                        mult *= 1.0 + 0.2 * n;
                        clearBuffs(at(t));
                        log.push_back(nameOf(t.side, t.slot) + " loses its buffs!");
                    }
                    break;
                }
                default: break;
            }
            const double power = shot.second > 0 ? shot.second : 70.0;
            const int dm = dealHit(user, t, power, m, mult, fixed, focusUsed);
            if (dm < 0) continue;
            anyHit = true;
            landed.push_back(t);
            if (m.perHitFx) {
                for (const Effect& e : m.fx)
                    if (!e.self) applyEffect(e, t, user);
            }
        }
        if (!m.perHitFx) {
            std::vector<Ref> done;
            for (const Ref& t : landed) {
                bool seen = false;
                for (const Ref& d : done) if (d.side == t.side && d.slot == t.slot) seen = true;
                if (seen) continue;
                done.push_back(t);
                for (const Effect& e : m.fx)
                    if (!e.self) applyEffect(e, t, user);
            }
        }
        if (m.sp == Sp::ChaosBolt && anyHit && !landed.empty()) {
            const Ref t = landed.back();
            switch (rnd(5)) {
                case 0: applyEffect({EK::Wound, 2}, t, user); break;
                case 1: applyEffect({EK::Slow, 1}, t, user); break;
                case 2: applyEffect({EK::Daze, 1}, t, user); break;
                case 3: applyEffect({EK::Mark, 0}, t, user); break;
                default: applyEffect({EK::Focus, 0}, user, user); break;
            }
        }
        if (focusUsed || f.focus) {
            f.focus = false;
            if (f.empowerN > 0 && --f.empowerN == 0) f.empowerPct = 0;
        }
        if (m.recoil > 0 && anyHit && f.alive()) f.hp = std::max(1, f.hp - std::max(1, f.maxHp * m.recoil / 100));
    } else {
        // Utility moves
        switch (m.sp) {
            case Sp::ResetCooldowns:
                for (int& c : f.cd) c = 0;
                log.push_back(nameOf(user.side, user.slot) + " rewinds time.");
                break;
            case Sp::Anarchy:
                for (int s = 0; s < 2; ++s)
                    for (int k = 0; k < kActiveSlots; ++k)
                        if (slotFilled(s, k) && fighter(s, k).alive()) {
                            Fighter& x = fighter(s, k);
                            const bool sleepy = x.sleepy;
                            const int hp = x.hp;
                            x.clearStatuses();
                            x.sleepy = sleepy;
                            x.hp = hp;
                            for (int& c : x.cd) c = 0;
                        }
                log.push_back("Chaos! Every status and cooldown is wiped.");
                break;
            case Sp::RebirthPyre:
                f.rebirth = true;
                log.push_back(nameOf(user.side, user.slot) + " is wreathed in flame.");
                break;
            case Sp::WildCard: {
                const Ref any = randomEnemy(user.side);  // safe: the guard above ensures a living foe exists
                switch (rnd(4)) {
                    case 0: heal(user, f.maxHp * 25 / 100); log.push_back("Wild Card: a healing glow!"); break;
                    case 1: {
                        bool fu = false;
                        Move heavy("Wild Strike", 110);
                        if (at(any).alive()) dealHit(user, any, 110, heavy, scale, 0, fu);
                        break;
                    }
                    case 2: applyEffect({EK::Haste, 2}, user, user); break;
                    default: applyEffect({EK::Shield, 20}, user, user); break;
                }
                break;
            }
            case Sp::StealBuff: case Sp::InvertBuffs: case Sp::CopyMods: {
                Ref t = primary;
                if (t.side == user.side || !slotFilled(t.side, t.slot) || !at(t).alive()) t = randomEnemy(user.side);
                Fighter& tf = at(t);
                if (m.sp == Sp::StealBuff) {
                    if (tf.empowerN > 0) { f.empowerN = tf.empowerN; f.empowerPct = tf.empowerPct; tf.empowerN = tf.empowerPct = 0; }
                    else if (tf.focus) { tf.focus = false; f.focus = true; }
                    else if (tf.haste > 0) { f.haste = tf.haste; tf.haste = 0; }
                    else if (tf.armor > 0) { f.armor = tf.armor; tf.armor = 0; }
                    else if (tf.regen > 0) { f.regen = tf.regen; tf.regen = 0; }
                    else if (tf.thorns > 0) { f.thorns = tf.thorns; tf.thorns = 0; }
                    else if (tf.shield > 0) { f.shield = tf.shield; tf.shield = 0; }
                    else if (tf.atkMod > 0 && tf.atkModT > 0) { f.atkMod = tf.atkMod; f.atkModT = tf.atkModT; tf.atkModT = 0; }
                    else { log.push_back("But there was nothing to steal."); }
                } else if (m.sp == Sp::InvertBuffs) {
                    if (tf.haste > 0) { tf.slow = std::max(tf.slow, tf.haste); tf.haste = 0; }
                    if (tf.regen > 0) { tf.wound = std::max(tf.wound, tf.regen); tf.regen = 0; }
                    if (tf.armor > 0) { tf.defMod = -30; tf.defModT = tf.armor; tf.armor = 0; }
                    if (tf.atkMod > 0 && tf.atkModT > 0) tf.atkMod = -tf.atkMod;
                    if (tf.defMod > 0 && tf.defModT > 0) tf.defMod = -tf.defMod;
                    if (tf.spdMod > 0 && tf.spdModT > 0) tf.spdMod = -tf.spdMod;
                    tf.shield = 0; tf.focus = false; tf.empowerN = tf.empowerPct = 0; tf.thorns = 0;
                    log.push_back(nameOf(t.side, t.slot) + "'s buffs turn against it!");
                } else {
                    f.atkMod = tf.atkMod; f.atkModT = tf.atkModT;
                    f.defMod = tf.defMod; f.defModT = tf.defModT;
                    f.spdMod = tf.spdMod; f.spdModT = tf.spdModT;
                    f.haste = tf.haste; f.slow = tf.slow;
                    log.push_back(nameOf(user.side, user.slot) + " copies the target's stat changes.");
                }
                break;
            }
            default: break;
        }
        for (const Ref& t : targets)
            for (const Effect& e : m.fx)
                if (!e.self) applyEffect(e, t, user);
    }

    // Effects on the user (and leech, which records the target).
    for (const Effect& e : m.fx) {
        if (!e.self) continue;
        if (e.k == EK::Leech) {
            for (const Ref& t : targets) applyEffect(e, t, user);
        } else if (f.alive()) {
            applyEffect(e, user, user);
        }
    }

    if (hadNoMiss) f.noMiss = false;
    if (m.pivot && anyHit && f.alive()) pivot(user);
}

void Battle::execute(const Entry& e) {
    if (over()) return;
    // The fighter this action belonged to may have been swapped out earlier this turn.
    if (!slotFilled(e.who.side, e.who.slot) || sides[e.who.side].active[static_cast<size_t>(e.who.slot)] != e.teamIdx) return;
    Fighter& f = at(e.who);
    if (!f.alive()) return;
    if (e.act.type == Action::Switch) {
        doSwitch(e.who, e.act.bench);
        return;
    }
    f.actedThisTurn = true;
    if (f.stun > 0) {
        f.stun--;
        log.push_back(nameOf(e.who.side, e.who.slot) + " is stunned and can't move!");
        return;
    }

    int mi = e.act.move;
    Ref target = e.act.target;
    bool releasing = false;
    if (f.charging) {
        mi = f.chargeMove;
        target = f.chargeTarget;
        f.charging = false;
        releasing = true;
    } else if (mi < 0 || mi > 3 || !moveUsable(e.who.side, e.who.slot, mi)) {
        mi = 4;
        for (int i = 0; i < 4; ++i)
            if (moveUsable(e.who.side, e.who.slot, i)) { mi = i; break; }
    }

    if (mi == 4) {
        useMove(e.who, fallbackMove(), -1, target, 1.0);
        return;
    }
    const Move& m = f.sp().moves[static_cast<size_t>(mi)];
    if (m.charge && !releasing) {
        f.charging = true;
        f.chargeMove = mi;
        f.chargeTarget = target;
        f.lastMove = mi;
        if (m.chargeEvade) { f.evade = true; f.evadeChance = 100; }
        log.push_back(nameOf(e.who.side, e.who.slot) + " prepares " + m.name + "...");
        return;
    }
    useMove(e.who, m, mi, target, 1.0);
    // The user may have been swapped out by a pivot; cooldowns belong to the fighter, so look it up by team slot.
    f.lastMove = mi;
    f.cd[mi] = m.cd > 0 ? m.cd + 1 : 0;
    if (m.once) f.used[mi] = true;
}

void Battle::endOfTurn() {
    for (int s = 0; s < 2; ++s) {
        for (int k = 0; k < kActiveSlots; ++k) {
            if (!slotFilled(s, k)) continue;
            Fighter& f = fighter(s, k);
            if (!f.alive()) continue;
            const Ref r{s, k};
            const std::string n = nameOf(s, k);
            if (f.wound > 0) {
                const int d = std::max(1, f.maxHp * 6 / 100);
                log.push_back(n + " suffers " + std::to_string(d) + " from its wound.");
                hurt(r, d, true);
                if (!f.alive()) continue;
            }
            if (f.regen > 0) heal(r, std::max(1, f.maxHp * 8 / 100));
            if (f.leechTurns > 0) {
                if (f.wound > 0 && f.leechSrcSide >= 0) {
                    Fighter& src = sides[f.leechSrcSide].team[static_cast<size_t>(f.leechSrcIdx)];
                    if (src.alive()) src.hp = std::min(src.maxHp, src.hp + std::max(1, src.maxHp * 5 / 100));
                }
                f.leechTurns--;
            }
            if (f.doom > 0 && --f.doom == 0) {
                const int d = std::min(f.hp - 1, f.maxHp * 40 / 100);
                log.push_back(n + " is struck by doom!");
                if (d > 0) hurt(r, d, true);
            }
            if (f.daze > 0) f.daze--;
            if (f.slow > 0) f.slow--;
            if (f.haste > 0) f.haste--;
            if (f.armor > 0) f.armor--;
            if (f.thorns > 0) f.thorns--;
            if (f.regen > 0) f.regen--;
            if (f.taunt > 0) f.taunt--;
            if (f.trap > 0) f.trap--;
            if (f.seal > 0 && --f.seal == 0) f.sealMove = -1;
            if (f.atkModT > 0 && --f.atkModT == 0) f.atkMod = 0;
            if (f.defModT > 0 && --f.defModT == 0) f.defMod = 0;
            if (f.spdModT > 0 && --f.spdModT == 0) f.spdMod = 0;
            if (f.slowAttackers > 0) f.slowAttackers--;
            f.evade = false;
            f.counterMult = f.counterHitPower = f.counterWound = 0;
            f.counterAbsorb = false;
            f.actedThisTurn = false;
        }
    }
    // Fatigue: long stalemates wear everyone down, growing by 1% of max HP each turn after turn 30.
    if (turn > 30) {
        const int pct = turn - 30;
        for (int s = 0; s < 2; ++s)
            for (int k = 0; k < kActiveSlots; ++k)
                if (slotFilled(s, k) && fighter(s, k).alive())
                    hurt({s, k}, std::max(1, fighter(s, k).maxHp * pct / 100), true);
        if (turn == 31) log.push_back("Fatigue sets in! Everyone grows weary.");
    }
    for (auto& side : sides)
        for (auto& f : side.team)
            for (int& c : f.cd)
                if (c > 0) --c;

    // Remove the fainted from the field.
    for (int s = 0; s < 2; ++s)
        for (int k = 0; k < kActiveSlots; ++k)
            if (slotFilled(s, k) && !fighter(s, k).alive()) {
                fighter(s, k).clearStatuses();
                sides[s].active[static_cast<size_t>(k)] = -1;
            }
    checkWinner();
    if (over()) return;
    autoReplace(1);
    if (autoReplacePlayer) autoReplace(0);
}

void Battle::checkWinner() {
    auto alive = [](const Side& s) {
        for (const auto& f : s.team) if (f.alive()) return true;
        return false;
    };
    if (!alive(sides[0])) winner = 1;
    else if (!alive(sides[1])) winner = 0;
}

std::vector<int> Battle::emptyPlayerSlotsNeedingReplacement() const {
    std::vector<int> out;
    if (over()) return out;
    const bool hasBench = !benchAlive(0).empty();
    for (int k = 0; k < kActiveSlots; ++k)
        if (!slotFilled(0, k) && hasBench) out.push_back(k);
    return out;
}

void Battle::replace(int side, int slot, int benchIdx) {
    Side& s = sides[side];
    if (s.active[static_cast<size_t>(slot)] >= 0) return;
    auto bench = benchAlive(side);
    if (std::find(bench.begin(), bench.end(), benchIdx) == bench.end()) return;
    s.active[static_cast<size_t>(slot)] = benchIdx;
    log.push_back(nameOf(side, slot) + " joins the battle!");
}

void Battle::autoReplace(int side) {
    for (int k = 0; k < kActiveSlots; ++k) {
        if (slotFilled(side, k)) continue;
        auto bench = benchAlive(side);
        if (bench.empty()) return;
        replace(side, k, bench[0]);
    }
}

bool Battle::tryCatch(int slot, double chance) {
    if (!wild || !slotFilled(1, slot) || !fighter(1, slot).alive()) return false;
    Fighter& f = fighter(1, slot);
    if (rnd(1000) / 1000.0 >= chance) {
        log.push_back("The charm fizzles. " + nameOf(1, slot) + " broke free!");
        return false;
    }
    log.push_back(nameOf(1, slot) + " became your friend!");
    caught.push_back({f.speciesId, f.level});
    f.hp = 0;
    f.clearStatuses();
    sides[1].active[static_cast<size_t>(slot)] = -1;
    checkWinner();
    if (!over()) autoReplace(1);
    return true;
}

// ---------------------------------------------------------------------------
// Turn resolution
// ---------------------------------------------------------------------------

void Battle::resolveTurn(const std::array<Action, 3>& playerActions) {
    if (over()) return;
    ++turn;
    log.push_back("-- Turn " + std::to_string(turn) + " --");

    std::vector<Entry> entries;
    for (int s = 0; s < 2; ++s) {
        for (int k = 0; k < kActiveSlots; ++k) {
            if (!slotFilled(s, k) || !fighter(s, k).alive()) continue;
            Fighter& f = fighter(s, k);
            Action a = s == 0 ? playerActions[static_cast<size_t>(k)] : aiChoose(s, k);
            if (a.type == Action::None) { a.type = Action::UseMove; a.move = 0; }
            Entry e;
            e.who = {s, k};
            e.teamIdx = sides[s].active[static_cast<size_t>(k)];
            e.act = a;
            e.spd = effSpd(f);
            e.tie = static_cast<uint32_t>(rng_());
            if (a.type == Action::Switch) {
                e.prio = 100;
            } else {
                int mi = f.charging ? f.chargeMove : a.move;
                e.prio = (mi >= 0 && mi <= 3) ? f.sp().moves[static_cast<size_t>(mi)].prio : 0;
                if (f.prioNext && !f.charging) { e.prio += 1; f.prioNext = false; }
            }
            entries.push_back(e);
        }
    }
    std::sort(entries.begin(), entries.end(), [](const Entry& a, const Entry& b) {
        if (a.prio != b.prio) return a.prio > b.prio;
        if (a.spd != b.spd) return a.spd > b.spd;
        return a.tie < b.tie;
    });
    for (const Entry& e : entries) execute(e);
    endOfTurn();
}

// ---------------------------------------------------------------------------
// AI
// ---------------------------------------------------------------------------

double Battle::scoreMove(int side, int slot, int mi, Ref& bestTarget) {
    const Fighter& f = fighter(side, slot);
    const Move& m = f.sp().moves[static_cast<size_t>(mi)];
    const auto enemies = livingEnemies(side);
    const auto allies = livingAllies(side);
    if (enemies.empty()) return 0;

    Ref strongest = enemies[0], weakest = enemies[0];
    for (const Ref& r : enemies) {
        const Fighter& e = at(r);
        if (effAtk(e) * e.hpPct() > effAtk(at(strongest)) * at(strongest).hpPct()) strongest = r;
        if (e.hp < at(weakest).hp) weakest = r;
    }
    bestTarget = strongest;

    const bool damaging = m.damaging() || m.sp == Sp::ShapeshiftSlam;
    if (damaging) {
        double totalPower = 0;
        if (m.hits.empty()) totalPower = m.power; else for (int h : m.hits) totalPower += h;
        if (m.sp == Sp::ShapeshiftSlam) totalPower = 60;
        double score = 0;
        if (m.tgt == Tgt::Spread) {
            for (const Ref& r : enemies) score += std::min(estimateDamage(f, at(r), totalPower) * 0.75, static_cast<double>(at(r).hp)) / at(r).maxHp * 100;
        } else if (m.randomTargets) {
            for (const Ref& r : enemies) score += std::min(estimateDamage(f, at(r), totalPower), static_cast<double>(at(r).hp)) / at(r).maxHp * 100 / enemies.size();
        } else {
            double best = -1;
            for (const Ref& r : enemies) {
                const double dmg = estimateDamage(f, at(r), totalPower);
                double s = std::min(dmg, static_cast<double>(at(r).hp)) / at(r).maxHp * 100;
                if (dmg >= at(r).hp) s += 30;
                if (s > best) { best = s; bestTarget = r; }
            }
            score = best;
        }
        score += 4.0 * static_cast<double>(m.fx.size());
        if (m.charge) score *= 0.8;
        if (m.recoil > 0) score -= 3;
        if (f.focus || f.empowerN > 0) score *= 1.3;
        return score;
    }

    // Utility moves: sum a heuristic value per effect.
    double score = 0;
    Ref ally = {side, slot};
    double lowest = f.hpPct();
    for (const Ref& r : allies)
        if (at(r).hpPct() < lowest) { lowest = at(r).hpPct(); ally = r; }
    bestTarget = m.tgt == Tgt::Ally ? ally : strongest;
    const Fighter& tf = (m.tgt == Tgt::Ally) ? at(ally) : at(strongest);
    const double lowPct = tf.hpPct();

    auto effScore = [&](const Effect& e, const Fighter& target, bool onUser) {
        switch (e.k) {
            case EK::Heal: return (1.0 - target.hpPct()) * e.a * 3.2;
            case EK::Shield: return target.shield > 0 ? 4.0 : 22.0 + (1.0 - target.hpPct()) * 15;
            case EK::Regen: return target.regen > 0 ? 2.0 : 12.0 + (1.0 - target.hpPct()) * 20;
            case EK::Armor: return target.armor > 0 ? 2.0 : 24.0;
            case EK::Thorns: return target.thorns > 0 ? 2.0 : 14.0;
            case EK::Focus: case EK::Empower: return (target.focus || target.empowerN > 0) ? 2.0 : 28.0;
            case EK::AtkMod: return onUser ? (e.a > 0 && target.atkModT == 0 ? 26.0 : 4.0) : (target.atkModT == 0 ? 20.0 : 4.0);
            case EK::DefMod: return target.defModT == 0 ? 20.0 : 4.0;
            case EK::SpdMod: return target.spdModT == 0 ? 14.0 : 4.0;
            case EK::Haste: return target.haste > 0 ? 2.0 : 18.0;
            case EK::Evade: return target.evade ? 0.0 : 14.0;
            case EK::Counter: case EK::CounterHit: case EK::CounterAbsorb: return 12.0;
            case EK::Wound: return target.wound > 0 ? 3.0 : 30.0;
            case EK::Daze: return target.daze > 0 ? 2.0 : 22.0;
            case EK::Slow: return target.slow > 0 ? 2.0 : 20.0;
            case EK::Mark: return target.mark ? 2.0 : 24.0;
            case EK::Trap: return target.trap > 0 ? 2.0 : 16.0;
            case EK::Stun: return 38.0;
            case EK::Doom: return target.doom > 0 ? 1.0 : 34.0;
            case EK::Seal: return target.lastMove >= 0 && target.seal == 0 ? 20.0 : 2.0;
            case EK::ForceSwitch: return benchAlive(1 - side).empty() ? 0.0 : 14.0;
            case EK::Cleanse: return (f.wound + f.daze + f.slow + f.stun + f.doom + (f.mark ? 1 : 0)) > 0 ? 34.0 : 1.0;
            case EK::SelfDamage: return -8.0;
            case EK::Plate: return f.plates >= 5 ? 0.0 : 18.0;
            case EK::SlowAttackers: return 8.0;
            case EK::ReflectDebuff: case EK::ReflectDamage: return 8.0;
            case EK::PrioNext: return 10.0;
            default: return 6.0;
        }
    };
    for (const Effect& e : m.fx) {
        const bool onUser = e.self || m.tgt == Tgt::Self;
        score += effScore(e, onUser ? f : tf, onUser) * e.chance / 100.0;
    }
    switch (m.sp) {
        case Sp::Mimic: score += at(strongest).lastMove >= 0 ? 24 : 0; break;
        case Sp::StealBuff: score += countBuffs(at(strongest)) > 0 ? 28 : 0; break;
        case Sp::InvertBuffs: score += countBuffs(at(strongest)) * 14; break;
        case Sp::ResetCooldowns: { int n = 0; for (int c : f.cd) n += c > 0; score += n * 14; break; }
        case Sp::WildCard: score += 20; break;
        case Sp::Anarchy: score += 8; break;
        case Sp::RebirthPyre: score += f.rebirth ? 0 : 26; break;
        case Sp::CopyMods: score += 8; break;
        default: break;
    }
    (void)lowPct;
    return score;
}

Action Battle::aiChoose(int side, int slot) {
    Action act;
    act.type = Action::UseMove;
    const Fighter& f = fighter(side, slot);
    if (f.charging) { act.move = f.chargeMove; act.target = f.chargeTarget; return act; }

    // Occasionally bail out a badly hurt friend.
    if (!smartAi && f.hpPct() < 0.2 && canSwitch(side, slot) && rnd(100) < 20) {
        auto bench = benchAlive(side);
        act.type = Action::Switch;
        act.bench = bench[static_cast<size_t>(rnd(static_cast<int>(bench.size())))];
        return act;
    }

    double bestScore = -1e9;
    int bestMove = -1;
    Ref bestTarget{1 - side, 0};
    for (int mi = 0; mi < 4; ++mi) {
        if (!moveUsable(side, slot, mi)) continue;
        Ref t{1 - side, 0};
        double s = scoreMove(side, slot, mi, t);
        if (!smartAi) s += rnd(15);
        if (s > bestScore) { bestScore = s; bestMove = mi; bestTarget = t; }
    }
    if (bestMove < 0) {
        act.move = 4;
        act.target = randomEnemy(side);
    } else {
        act.move = bestMove;
        act.target = bestTarget;
    }
    return act;
}

}  // namespace gf
