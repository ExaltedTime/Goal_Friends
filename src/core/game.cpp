#include "game.hpp"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace gf {

// ---------------------------------------------------------------------------
// Small helpers
// ---------------------------------------------------------------------------

int baseXp(Effort e) {
    switch (e) { case Effort::S: return 10; case Effort::M: return 25; case Effort::L: return 60; default: return 120; }
}
int energyFor(Effort e) {
    switch (e) { case Effort::S: return 1; case Effort::M: return 2; case Effort::L: return 4; default: return 8; }
}
int coinsFor(Effort e) { return baseXp(e) / 5; }
const char* effortName(Effort e) {
    switch (e) { case Effort::S: return "S"; case Effort::M: return "M"; case Effort::L: return "L"; default: return "XL"; }
}

namespace {
std::mt19937 makeRng(GameState& g) {
    std::mt19937 r(g.rngSeed);
    g.rngSeed = r();
    return r;
}
int pick(std::mt19937& r, int n) { return n <= 1 ? 0 : static_cast<int>(r() % static_cast<uint32_t>(n)); }
}  // namespace

const Task* findTask(const GameState& g, int id) {
    for (const auto& t : g.tasks) if (t.id == id) return &t;
    return nullptr;
}
Task* findTask(GameState& g, int id) {
    for (auto& t : g.tasks) if (t.id == id) return &t;
    return nullptr;
}
Friend* findFriend(GameState& g, int id) {
    for (auto& f : g.friends) if (f.id == id) return &f;
    return nullptr;
}
const Friend* findFriend(const GameState& g, int id) {
    for (const auto& f : g.friends) if (f.id == id) return &f;
    return nullptr;
}

int leadFriendId(const GameState& g) {
    if (!g.party.empty()) return g.party[0];
    if (!g.friends.empty()) return g.friends[0].id;
    return -1;
}

// ---------------------------------------------------------------------------
// Scheduling
// ---------------------------------------------------------------------------

DueWindow dueWindow(const Task& t) {
    int start = 0, end = 1439;
    if (t.targetMinute >= 0) {
        start = t.targetMinute;
        end = std::min(1439, start + 120);
    } else {
        switch (t.window) {
            case Window::Morning: start = 6 * 60; end = 12 * 60; break;
            case Window::Afternoon: start = 12 * 60; end = 17 * 60; break;
            case Window::Evening: start = 17 * 60; end = 22 * 60; break;
            default: break;
        }
    }
    return DueWindow{start, end, std::min(1439, end + 120)};
}

bool occursOn(const Task& t, int day) {
    const Recurrence& r = t.rec;
    switch (r.type) {
        case RecType::Once: return day == t.dueDay;
        case RecType::Daily: return true;
        case RecType::Weekdays: return (r.weekdayMask >> tu::weekday(day)) & 1;
        case RecType::EveryN: return r.n >= 1 && day >= r.anchorDay && (day - r.anchorDay) % r.n == 0;
        case RecType::Weekly: return tu::weekday(day) == r.weekday;
        case RecType::Monthly: {
            const tu::Civil c = tu::civilFromDays(day);
            const int dim = tu::daysInMonth(c.y, c.m);
            const int target = (r.monthday <= 0 || r.monthday > dim) ? dim : r.monthday;
            return c.d == target;
        }
    }
    return false;
}

static const Completion* findCompletion(const GameState& g, int taskId, int day) {
    for (const auto& c : g.completions) if (c.taskId == taskId && c.day == day) return &c;
    return nullptr;
}
static const Completion* findAnyCompletion(const GameState& g, int taskId) {
    for (const auto& c : g.completions) if (c.taskId == taskId) return &c;
    return nullptr;
}

std::vector<Occurrence> occurrencesOn(const GameState& g, int day, int nowMinute) {
    std::vector<Occurrence> out;
    for (const auto& t : g.tasks) {
        Occurrence o;
        o.taskId = t.id;
        o.day = day;
        o.win = dueWindow(t);
        if (t.rec.type == RecType::Once) {
            const Completion* c = findAnyCompletion(g, t.id);
            if (c) {
                if (c->day != day) continue;
                o.done = true;
                o.late = c->late;
            } else {
                if (day < t.dueDay) continue;
                o.overdue = day > t.dueDay;
            }
            out.push_back(o);
            continue;
        }
        if (t.paused || day < t.createdDay || !occursOn(t, day)) continue;
        if (const Completion* c = findCompletion(g, t.id, day)) {
            o.done = true;
            o.late = c->late;
        } else {
            o.missed = nowMinute < 0 || nowMinute > o.win.graceEnd;
        }
        out.push_back(o);
    }
    return out;
}

bool isSleepy(const GameState& g, int today) { return g.everActive && today - g.lastActiveDay >= 2; }

void sweep(GameState& g, tu::Time now) {
    const int today = now.day;
    if (g.lastSweepDay < -100000) {
        g.lastSweepDay = today - 1;
    } else {
        const int from = std::max(g.lastSweepDay + 1, today - 400);
        for (int d = from; d <= today - 1; ++d) {
            for (auto& t : g.tasks) {
                if (t.rec.type == RecType::Once || t.paused || t.createdDay > d || !occursOn(t, d)) continue;
                if (!findCompletion(g, t.id, d)) t.streak = 0;
            }
        }
        g.lastSweepDay = today - 1;
    }
    if (g.everActive && today - g.lastActiveDay >= 2) g.globalStreak = 0;
}

int streakBonusPct(const Task& t, const GameState& g) {
    return std::min(50, 5 * t.streak) + std::min(20, 2 * g.globalStreak);
}

// ---------------------------------------------------------------------------
// Friends, XP, eggs
// ---------------------------------------------------------------------------

Result addXp(GameState& g, int friendId, int xp) {
    Result r;
    Friend* f = findFriend(g, friendId);
    if (!f) return r;
    r.ok = true;
    f->xp += xp;
    while (f->level < kMaxLevel && f->xp >= xpToNext(f->level)) {
        f->xp -= xpToNext(f->level);
        f->level++;
        r.messages.push_back(species(f->species).name + " reached level " + std::to_string(f->level) + "!");
        for (;;) {
            const Species& s = species(f->species);
            if (s.evolvesTo < 0 || f->level < s.evolveLevel) break;
            const std::string old = s.name;
            f->species = s.evolvesTo;
            g.dexOwned.insert(f->species);
            r.messages.push_back(old + " evolved into " + species(f->species).name + "!");
        }
    }
    if (f->level >= kMaxLevel) f->xp = 0;
    return r;
}

Result addFriend(GameState& g, int speciesId, int level) {
    Result r;
    r.ok = true;
    if (g.dexOwned.count(speciesId)) {
        const int c = 15 + level;
        g.coins += c;
        r.messages.push_back("Already have " + species(speciesId).name + ": converted to " + std::to_string(c) + " coins.");
        return r;
    }
    Friend f;
    f.id = g.nextFriendId++;
    f.species = speciesId;
    f.level = std::max(1, level);
    g.friends.push_back(f);
    g.dexOwned.insert(speciesId);
    g.dexSeen.insert(speciesId);
    if (g.party.size() < static_cast<size_t>(kPartySize)) g.party.push_back(f.id);
    r.messages.push_back(species(speciesId).name + " joined your friends!");
    return r;
}

Result setPartyMember(GameState& g, int friendId, bool inParty) {
    Result r;
    auto it = std::find(g.party.begin(), g.party.end(), friendId);
    if (inParty) {
        if (it != g.party.end()) { r.ok = true; return r; }
        if (g.party.size() >= static_cast<size_t>(kPartySize)) { r.messages.push_back("Your party is full (6)."); return r; }
        if (!findFriend(g, friendId)) return r;
        g.party.push_back(friendId);
    } else {
        if (it == g.party.end()) { r.ok = true; return r; }
        if (g.party.size() <= 1) { r.messages.push_back("You need at least one friend in the party."); return r; }
        g.party.erase(it);
    }
    r.ok = true;
    return r;
}

static int hatchSpecies(GameState& g) {
    auto rng = makeRng(g);
    const auto bases = baseSpecies();
    std::vector<int> common, rare;
    for (int id : bases) (species(id).rare ? rare : common).push_back(id);
    int chosen = -1;
    for (int attempt = 0; attempt < 4; ++attempt) {
        const bool wantRare = pick(rng, 100) < 6;
        const auto& pool = (wantRare && !rare.empty()) ? rare : common;
        chosen = pool[static_cast<size_t>(pick(rng, static_cast<int>(pool.size())))];
        if (!g.dexOwned.count(chosen)) break;
    }
    return chosen;
}

Result chooseStarter(GameState& g, int speciesId, tu::Time now) {
    Result r;
    if (g.started || !species(speciesId).starter) return r;
    g.started = true;
    r = addFriend(g, speciesId, 1);
    g.eggs.push_back(Egg{});
    g.coins += 20;
    g.lastSweepDay = now.day - 1;
    r.messages.push_back("You also received an Egg and 20 coins. Finish tasks to hatch it!");
    r.ok = true;
    return r;
}

Result buyEgg(GameState& g) {
    Result r;
    if (g.coins < kEggCost) { r.messages.push_back("Not enough coins."); return r; }
    g.coins -= kEggCost;
    g.eggs.push_back(Egg{});
    r.ok = true;
    r.messages.push_back("Bought an Egg. It hatches after 5 completed tasks.");
    return r;
}

Result buyCharm(GameState& g) {
    Result r;
    if (g.coins < kCharmCost) { r.messages.push_back("Not enough coins."); return r; }
    g.coins -= kCharmCost;
    g.charms++;
    r.ok = true;
    r.messages.push_back("Bought a Catch Charm.");
    return r;
}

// ---------------------------------------------------------------------------
// Tasks
// ---------------------------------------------------------------------------

int addTask(GameState& g, Task t, tu::Time now) {
    t.id = g.nextTaskId++;
    t.createdDay = now.day;
    if (t.rec.type == RecType::Once && t.dueDay == 0) t.dueDay = now.day;
    if (t.rec.type == RecType::EveryN) { if (t.rec.n < 1) t.rec.n = 1; t.rec.anchorDay = now.day; }
    g.tasks.push_back(std::move(t));
    return g.nextTaskId - 1;
}

void removeTask(GameState& g, int taskId) {
    g.tasks.erase(std::remove_if(g.tasks.begin(), g.tasks.end(), [&](const Task& t) { return t.id == taskId; }), g.tasks.end());
}

static void markActive(GameState& g, int day) {
    if (g.lastActiveDay == day) return;
    if (g.lastActiveDay == day - 1) g.globalStreak++;
    else g.globalStreak = 1;
    g.lastActiveDay = day;
    g.everActive = true;
}

Result completeTask(GameState& g, int taskId, tu::Time now) {
    Result r;
    Task* t = findTask(g, taskId);
    if (!t) return r;
    const bool once = t->rec.type == RecType::Once;
    if (once) {
        if (findAnyCompletion(g, taskId)) { r.messages.push_back("Already done."); return r; }
    } else {
        if (t->paused || !occursOn(*t, now.day) || now.day < t->createdDay) { r.messages.push_back("This task isn't due today."); return r; }
        if (findCompletion(g, taskId, now.day)) { r.messages.push_back("Already done today."); return r; }
    }
    const DueWindow w = dueWindow(*t);
    const bool late = !once && w.end < 1439 && now.minute > w.end;

    const int bonus = streakBonusPct(*t, g);
    double xp = baseXp(t->effort) * (1.0 + bonus / 100.0);
    if (late) xp *= 0.5;
    const int xpInt = std::max(1, static_cast<int>(xp));

    if (!once) t->streak++;
    markActive(g, now.day);
    g.totalTasksDone++;
    g.completions.push_back(Completion{taskId, now.day, now.minute, late});

    const int energy = energyFor(t->effort);
    const int coins = coinsFor(t->effort);
    g.energy += energy;
    g.coins += coins;
    r.ok = true;
    r.messages.push_back("Done: " + t->title + (late ? " (late, half XP)" : ""));
    std::string reward = "+" + std::to_string(xpInt) + " XP, +" + std::to_string(energy) + " Energy, +" + std::to_string(coins) + " coins";
    if (bonus > 0) reward += " (streak +" + std::to_string(bonus) + "%)";
    r.messages.push_back(reward);

    int target = t->assignedTo;
    if (!findFriend(g, target)) target = leadFriendId(g);
    if (target >= 0) {
        const Result xr = addXp(g, target, xpInt);
        r.messages.insert(r.messages.end(), xr.messages.begin(), xr.messages.end());
    }

    for (size_t i = 0; i < g.eggs.size();) {
        if (++g.eggs[i].progress >= g.eggs[i].needed) {
            g.eggs.erase(g.eggs.begin() + static_cast<long>(i));
            const int sid = hatchSpecies(g);
            r.messages.push_back("An egg hatched!");
            const Result hr = addFriend(g, sid, 1);
            r.messages.insert(r.messages.end(), hr.messages.begin(), hr.messages.end());
        } else {
            ++i;
        }
    }
    return r;
}

Result completeFocus(GameState& g, int minutes, tu::Time now) {
    Result r;
    if (minutes <= 0) return r;
    markActive(g, now.day);
    const int energy = std::max(1, (minutes + 9) / 10);
    g.energy += energy;
    r.ok = true;
    r.messages.push_back("Focus session complete! +" + std::to_string(energy) + " Energy.");
    const int lead = leadFriendId(g);
    if (lead >= 0) {
        const Result xr = addXp(g, lead, minutes);
        r.messages.push_back("+" + std::to_string(minutes) + " XP to " + species(findFriend(g, lead)->species).name + ".");
        r.messages.insert(r.messages.end(), xr.messages.begin(), xr.messages.end());
    }
    return r;
}

// ---------------------------------------------------------------------------
// Reminders
// ---------------------------------------------------------------------------

bool inQuietHours(const Settings& s, int minute) {
    if (s.quietStart == s.quietEnd) return false;
    if (s.quietStart > s.quietEnd) return minute >= s.quietStart || minute < s.quietEnd;
    return minute >= s.quietStart && minute < s.quietEnd;
}

static ReminderState& reminderFor(GameState& g, int taskId, int day) {
    for (auto& r : g.reminders) if (r.taskId == taskId && r.day == day) return r;
    g.reminders.push_back(ReminderState{taskId, day, 0, 0, false});
    return g.reminders.back();
}

std::vector<ReminderMsg> pollReminders(GameState& g, tu::Time now) {
    std::vector<ReminderMsg> out;
    g.reminders.erase(std::remove_if(g.reminders.begin(), g.reminders.end(), [&](const ReminderState& r) { return r.day < now.day - 2; }), g.reminders.end());
    if (inQuietHours(g.settings, now.minute)) return out;

    for (const auto& t : g.tasks) {
        if (!t.remind || t.paused || (t.targetMinute < 0 && t.window == Window::Any)) continue;
        if (t.rec.type != RecType::Once ? (now.day < t.createdDay || !occursOn(t, now.day)) : (now.day != t.dueDay)) continue;
        if (t.rec.type == RecType::Once ? findAnyCompletion(g, t.id) != nullptr : findCompletion(g, t.id, now.day) != nullptr) continue;
        const DueWindow w = dueWindow(t);
        if (now.minute > w.graceEnd) continue;

        ReminderState& st = reminderFor(g, t.id, now.day);
        if (st.dismissed) continue;
        if (st.snoozeUntil > 0) {
            if (now.abs() < st.snoozeUntil) continue;
            st.snoozeUntil = 0;
            out.push_back({t.id, "Snoozed reminder: " + t.title});
            continue;
        }
        std::vector<int> times = {w.start, w.start + 30, w.start + 120};
        if (w.end < 1439 && w.end - 15 > w.start + 120) times.push_back(w.end - 15);
        int latest = -1;
        for (size_t i = 0; i < times.size(); ++i)
            if (times[i] <= now.minute && static_cast<int>(i) >= st.stage) latest = static_cast<int>(i);
        if (latest < 0) continue;
        out.push_back({t.id, (latest == 0 ? "Time for: " : "Still to do: ") + t.title});
        st.stage = latest + 1;
    }
    return out;
}

void snoozeReminder(GameState& g, int taskId, int day, tu::Time now, int minutes) {
    ReminderState& st = reminderFor(g, taskId, day);
    st.snoozeUntil = now.abs() + minutes;
}

void dismissReminder(GameState& g, int taskId, int day) { reminderFor(g, taskId, day).dismissed = true; }

// ---------------------------------------------------------------------------
// Battles
// ---------------------------------------------------------------------------

int energyCost(BattleMode m) { return m == BattleMode::Wild ? 1 : m == BattleMode::Duel ? 2 : 5; }
bool bossUnlocked(const GameState& g) { return g.globalStreak >= 3; }

namespace {
int entryLevel(const Species& s) {
    if (s.stage <= 1) return 1;
    for (const auto& p : allSpecies())
        if (p.evolvesTo == s.id) return p.evolveLevel;
    return 1;
}

// One species per line that is valid at this level (its current evolution form), plus optionally rares.
std::vector<int> candidatesFor(int level, bool rares, bool finalsOnly) {
    std::vector<int> out;
    for (const auto& s : allSpecies()) {
        if (s.rare) { if (rares && level >= 5) out.push_back(s.id); continue; }
        if (finalsOnly) { if (s.evolvesTo < 0) out.push_back(s.id); continue; }
        if (entryLevel(s) <= level && (s.evolvesTo < 0 || level < s.evolveLevel)) out.push_back(s.id);
    }
    return out;
}
}  // namespace

bool startBattle(GameState& g, BattleMode mode, tu::Time now, std::unique_ptr<Battle>& out, std::string& why) {
    std::vector<Fighter> mine;
    int levelSum = 0;
    const bool sleepy = isSleepy(g, now.day);
    for (int id : g.party) {
        const Friend* f = findFriend(g, id);
        if (!f) continue;
        mine.push_back(makeFighter(f->species, f->level, sleepy, f->id));
        levelSum += f->level;
    }
    if (mine.empty()) { why = "You need at least one friend first."; return false; }
    if (mode == BattleMode::Boss && !bossUnlocked(g)) { why = "Bosses unlock with a 3-day streak."; return false; }
    const int cost = energyCost(mode);
    if (g.energy < cost) { why = "Need " + std::to_string(cost) + " Energy (you have " + std::to_string(g.energy) + ")."; return false; }
    g.energy -= cost;

    auto rng = makeRng(g);
    const int avg = std::max(1, static_cast<int>(std::lround(static_cast<double>(levelSum) / static_cast<double>(mine.size()))));
    std::vector<Fighter> foes;
    int count = 1;
    switch (mode) {
        case BattleMode::Wild: count = 1 + pick(rng, 3); break;
        case BattleMode::Duel: count = static_cast<int>(mine.size()); break;
        case BattleMode::Boss: count = std::max(3, static_cast<int>(mine.size())); break;
    }
    count = std::min(count, kPartySize);
    for (int i = 0; i < count; ++i) {
        int level;
        std::vector<int> pool;
        if (mode == BattleMode::Boss) {
            level = std::min(kMaxLevel, avg + 4);
            pool = candidatesFor(std::max(level, 33), true, true);
        } else {
            level = std::max(1, avg + (mode == BattleMode::Wild ? pick(rng, 3) - 1 : pick(rng, 3)));
            pool = candidatesFor(level, pick(rng, 100) < 8, false);
        }
        const int sid = pool[static_cast<size_t>(pick(rng, static_cast<int>(pool.size())))];
        foes.push_back(makeFighter(sid, level));
        g.dexSeen.insert(sid);
    }
    out = std::make_unique<Battle>(std::move(mine), std::move(foes), static_cast<uint32_t>(rng()), mode == BattleMode::Wild, mode == BattleMode::Boss);
    out->autoReplacePlayer = false;
    return true;
}

double catchChance(const Fighter& foe) {
    return std::clamp(0.25 + 0.55 * (1.0 - foe.hpPct()), 0.1, 0.9);
}

bool useCharm(GameState& g, Battle& b, int foeSlot) {
    if (g.charms <= 0 || !b.wild || !b.slotFilled(1, foeSlot)) return false;
    g.charms--;
    return b.tryCatch(foeSlot, catchChance(b.fighter(1, foeSlot)));
}

Result finishBattle(GameState& g, const Battle& b, BattleMode mode) {
    Result r;
    r.ok = true;
    for (const auto& c : b.caught) {
        const Result cr = addFriend(g, c.species, c.level);
        r.messages.insert(r.messages.end(), cr.messages.begin(), cr.messages.end());
    }
    if (!b.over()) return r;
    g.battlesPlayed++;
    if (b.winner != 0) {
        r.messages.push_back("You were defeated. Your friends are okay and fully healed.");
        return r;
    }
    g.battlesWon++;
    int lvlSum = 0, n = 0;
    for (const auto& f : b.sides[1].team) { lvlSum += f.level; ++n; }
    const int avg = n > 0 ? lvlSum / n : 1;
    int coins = 0;
    switch (mode) {
        case BattleMode::Wild: coins = 4 * n + avg; break;
        case BattleMode::Duel: coins = 20 + 2 * avg; break;
        case BattleMode::Boss: coins = 60 + 3 * avg; break;
    }
    g.coins += coins;
    r.messages.push_back("Victory! +" + std::to_string(coins) + " coins.");
    auto rng = makeRng(g);
    if (mode == BattleMode::Duel && pick(rng, 100) < 35) {
        g.eggs.push_back(Egg{});
        r.messages.push_back("The trainer gave you an Egg!");
    }
    if (mode == BattleMode::Boss) {
        g.bossesWon++;
        std::vector<int> unowned;
        for (const auto& s : allSpecies())
            if (s.rare && !g.dexOwned.count(s.id)) unowned.push_back(s.id);
        if (unowned.empty()) {
            g.coins += 100;
            r.messages.push_back("Boss reward: +100 coins.");
        } else {
            const int sid = unowned[static_cast<size_t>(pick(rng, static_cast<int>(unowned.size())))];
            const Result cr = addFriend(g, sid, std::max(5, avg - 2));
            r.messages.insert(r.messages.end(), cr.messages.begin(), cr.messages.end());
        }
    }
    return r;
}

// ---------------------------------------------------------------------------
// Save / load
// ---------------------------------------------------------------------------

json::Value toJson(const GameState& g) {
    using json::Value;
    Value o = Value::object();
    o["version"] = g.version;
    o["started"] = g.started;
    o["energy"] = g.energy;
    o["coins"] = g.coins;
    o["charms"] = g.charms;
    o["globalStreak"] = g.globalStreak;
    o["lastActiveDay"] = g.lastActiveDay;
    o["everActive"] = g.everActive;
    o["nextTaskId"] = g.nextTaskId;
    o["nextFriendId"] = g.nextFriendId;
    o["lastSweepDay"] = g.lastSweepDay;
    o["totalTasksDone"] = g.totalTasksDone;
    o["bossesWon"] = g.bossesWon;
    o["battlesWon"] = g.battlesWon;
    o["battlesPlayed"] = g.battlesPlayed;
    o["rngSeed"] = static_cast<long long>(g.rngSeed);

    Value st = Value::object();
    st["confirmDone"] = g.settings.confirmDone;
    st["quietStart"] = g.settings.quietStart;
    st["quietEnd"] = g.settings.quietEnd;
    o["settings"] = st;

    Value tasks = Value::array();
    for (const auto& t : g.tasks) {
        Value v = Value::object();
        v["id"] = t.id;
        v["title"] = t.title;
        v["effort"] = static_cast<int>(t.effort);
        v["category"] = t.category;
        v["assignedTo"] = t.assignedTo;
        v["targetMinute"] = t.targetMinute;
        v["window"] = static_cast<int>(t.window);
        v["remind"] = t.remind;
        v["paused"] = t.paused;
        v["streak"] = t.streak;
        v["createdDay"] = t.createdDay;
        v["dueDay"] = t.dueDay;
        v["recType"] = static_cast<int>(t.rec.type);
        v["recMask"] = t.rec.weekdayMask;
        v["recN"] = t.rec.n;
        v["recWeekday"] = t.rec.weekday;
        v["recMonthday"] = t.rec.monthday;
        v["recAnchor"] = t.rec.anchorDay;
        tasks.push(v);
    }
    o["tasks"] = tasks;

    Value comps = Value::array();
    for (const auto& c : g.completions) {
        Value v = Value::array();
        v.push(c.taskId); v.push(c.day); v.push(c.minute); v.push(c.late);
        comps.push(v);
    }
    o["completions"] = comps;

    Value friends = Value::array();
    for (const auto& f : g.friends) {
        Value v = Value::object();
        v["id"] = f.id; v["species"] = f.species; v["level"] = f.level; v["xp"] = f.xp;
        friends.push(v);
    }
    o["friends"] = friends;

    Value party = Value::array();
    for (int id : g.party) party.push(id);
    o["party"] = party;

    Value eggs = Value::array();
    for (const auto& e : g.eggs) {
        Value v = Value::object();
        v["progress"] = e.progress; v["needed"] = e.needed;
        eggs.push(v);
    }
    o["eggs"] = eggs;

    Value owned = Value::array(), seen = Value::array();
    for (int s : g.dexOwned) owned.push(s);
    for (int s : g.dexSeen) seen.push(s);
    o["dexOwned"] = owned;
    o["dexSeen"] = seen;

    Value rem = Value::array();
    for (const auto& r : g.reminders) {
        Value v = Value::object();
        v["taskId"] = r.taskId; v["day"] = r.day; v["stage"] = r.stage;
        v["snoozeUntil"] = r.snoozeUntil; v["dismissed"] = r.dismissed;
        rem.push(v);
    }
    o["reminders"] = rem;
    return o;
}

bool fromJson(const json::Value& o, GameState& g) {
    if (o.type != json::Value::Type::Object) return false;
    GameState n;
    n.version = o.at("version").asInt(1);
    n.started = o.at("started").asBool();
    n.energy = o.at("energy").asInt();
    n.coins = o.at("coins").asInt();
    n.charms = o.at("charms").asInt();
    n.globalStreak = o.at("globalStreak").asInt();
    n.lastActiveDay = o.at("lastActiveDay").asInt(-1000000);
    n.everActive = o.at("everActive").asBool();
    n.nextTaskId = o.at("nextTaskId").asInt(1);
    n.nextFriendId = o.at("nextFriendId").asInt(1);
    n.lastSweepDay = o.at("lastSweepDay").asInt(-1000000);
    n.totalTasksDone = o.at("totalTasksDone").asInt();
    n.bossesWon = o.at("bossesWon").asInt();
    n.battlesWon = o.at("battlesWon").asInt();
    n.battlesPlayed = o.at("battlesPlayed").asInt();
    n.rngSeed = static_cast<uint32_t>(o.at("rngSeed").asInt64(12345));
    n.settings.confirmDone = o.at("settings").at("confirmDone").asBool(true);
    n.settings.quietStart = o.at("settings").at("quietStart").asInt(22 * 60);
    n.settings.quietEnd = o.at("settings").at("quietEnd").asInt(8 * 60);

    for (const auto& v : o.at("tasks").arr()) {
        Task t;
        t.id = v.at("id").asInt();
        t.title = v.at("title").asStr();
        t.effort = static_cast<Effort>(std::clamp(v.at("effort").asInt(1), 0, 3));
        t.category = v.at("category").asStr();
        t.assignedTo = v.at("assignedTo").asInt(-1);
        t.targetMinute = v.at("targetMinute").asInt(-1);
        t.window = static_cast<Window>(std::clamp(v.at("window").asInt(0), 0, 3));
        t.remind = v.at("remind").asBool(true);
        t.paused = v.at("paused").asBool();
        t.streak = v.at("streak").asInt();
        t.createdDay = v.at("createdDay").asInt();
        t.dueDay = v.at("dueDay").asInt();
        t.rec.type = static_cast<RecType>(std::clamp(v.at("recType").asInt(), 0, 5));
        t.rec.weekdayMask = v.at("recMask").asInt();
        t.rec.n = v.at("recN").asInt(1);
        t.rec.weekday = v.at("recWeekday").asInt(1);
        t.rec.monthday = v.at("recMonthday").asInt(1);
        t.rec.anchorDay = v.at("recAnchor").asInt();
        n.tasks.push_back(std::move(t));
    }
    for (const auto& v : o.at("completions").arr()) {
        if (v.arr().size() < 4) continue;
        n.completions.push_back(Completion{v.arr()[0].asInt(), v.arr()[1].asInt(), v.arr()[2].asInt(), v.arr()[3].asBool()});
    }
    for (const auto& v : o.at("friends").arr()) {
        Friend f;
        f.id = v.at("id").asInt();
        f.species = std::clamp(v.at("species").asInt(), 0, kSpeciesCount - 1);
        f.level = std::clamp(v.at("level").asInt(1), 1, kMaxLevel);
        f.xp = v.at("xp").asInt();
        n.friends.push_back(f);
    }
    for (const auto& v : o.at("party").arr()) n.party.push_back(v.asInt());
    for (const auto& v : o.at("eggs").arr()) n.eggs.push_back(Egg{v.at("progress").asInt(), v.at("needed").asInt(5)});
    for (const auto& v : o.at("dexOwned").arr()) n.dexOwned.insert(v.asInt());
    for (const auto& v : o.at("dexSeen").arr()) n.dexSeen.insert(v.asInt());
    for (const auto& v : o.at("reminders").arr())
        n.reminders.push_back(ReminderState{v.at("taskId").asInt(), v.at("day").asInt(), v.at("stage").asInt(), v.at("snoozeUntil").asInt64(), v.at("dismissed").asBool()});
    g = std::move(n);
    return true;
}

bool saveToFile(const GameState& g, const std::string& path) {
    namespace fs = std::filesystem;
    std::error_code ec;
    const fs::path p(path);
    if (p.has_parent_path()) fs::create_directories(p.parent_path(), ec);
    const std::string tmp = path + ".tmp";
    {
        std::ofstream f(tmp, std::ios::binary | std::ios::trunc);
        if (!f) return false;
        f << json::dump(toJson(g), 1);
        f.flush();
        if (!f) return false;
    }
    fs::rename(tmp, p, ec);
    if (ec) {
        fs::remove(p, ec);
        ec.clear();
        fs::rename(tmp, p, ec);
    }
    return !ec;
}

bool loadFromFile(const std::string& path, GameState& g) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;
    std::stringstream ss;
    ss << f.rdbuf();
    json::Value v;
    if (!json::parse(ss.str(), v)) return false;
    return fromJson(v, g);
}

}  // namespace gf
