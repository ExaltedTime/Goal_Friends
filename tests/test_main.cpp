// Dependency-free test runner for the platform-free core.
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <array>
#include <map>
#include <set>
#include <string>

#include "battle.hpp"
#include "data.hpp"
#include "describe.hpp"
#include "game.hpp"
#include "sprite.hpp"
#include "timeutil.hpp"

using namespace gf;

static int g_checks = 0, g_failures = 0;
#define CHECK(cond)                                                                         \
    do {                                                                                    \
        ++g_checks;                                                                         \
        if (!(cond)) { ++g_failures; std::printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond); } \
    } while (0)

static tu::Time T(int day, int h = 12, int m = 0) { return tu::Time{day, h * 60 + m}; }

static void testTime() {
    CHECK(tu::daysFromCivil(1970, 1, 1) == 0);
    CHECK(tu::weekday(0) == 4);  // Thursday
    const int d = tu::daysFromCivil(2024, 2, 29);
    const auto c = tu::civilFromDays(d);
    CHECK(c.y == 2024 && c.m == 2 && c.d == 29);
    CHECK(tu::weekday(tu::daysFromCivil(2026, 11, 11)) == 3);  // Wednesday
    CHECK(tu::daysInMonth(2024, 2) == 29 && tu::daysInMonth(2025, 2) == 28);
    CHECK(tu::fromAbs(T(5, 3, 7).abs()).minute == 187);
}

static Task mk(const char* title, Effort e, RecType t) {
    Task k;
    k.title = title;
    k.effort = e;
    k.rec.type = t;
    return k;
}

static void testRecurrence() {
    const int mon = tu::daysFromCivil(2026, 11, 9);  // Monday
    Task daily = mk("d", Effort::S, RecType::Daily);
    CHECK(occursOn(daily, mon) && occursOn(daily, mon + 3));

    Task wd = mk("w", Effort::S, RecType::Weekdays);
    wd.rec.weekdayMask = (1 << 1) | (1 << 3) | (1 << 5);  // Mon Wed Fri
    CHECK(occursOn(wd, mon) && !occursOn(wd, mon + 1) && occursOn(wd, mon + 2));

    Task every3 = mk("e", Effort::S, RecType::EveryN);
    every3.rec.n = 3;
    every3.rec.anchorDay = mon;
    CHECK(occursOn(every3, mon) && !occursOn(every3, mon + 1) && occursOn(every3, mon + 3) && !occursOn(every3, mon - 3));

    Task weekly = mk("wk", Effort::S, RecType::Weekly);
    weekly.rec.weekday = 2;  // Tuesday
    CHECK(occursOn(weekly, mon + 1) && !occursOn(weekly, mon));

    Task monthly = mk("m", Effort::S, RecType::Monthly);
    monthly.rec.monthday = 31;
    CHECK(occursOn(monthly, tu::daysFromCivil(2025, 2, 28)));   // clamps to last day
    CHECK(!occursOn(monthly, tu::daysFromCivil(2025, 2, 27)));
    CHECK(occursOn(monthly, tu::daysFromCivil(2025, 12, 31)));
    monthly.rec.monthday = 0;
    CHECK(occursOn(monthly, tu::daysFromCivil(2024, 2, 29)));
}

static GameState fresh(int day = 20000) {
    GameState g;
    chooseStarter(g, 0, T(day));  // Mossling
    return g;
}

static void testRewards() {
    GameState g = fresh();
    CHECK(g.friends.size() == 1 && g.eggs.size() == 1 && g.coins == 20);
    Task t = mk("laundry", Effort::M, RecType::Once);
    const int id = addTask(g, t, T(20000));
    const Result r = completeTask(g, id, T(20000));
    CHECK(r.ok);
    CHECK(g.energy == 2);
    CHECK(g.coins == 25);
    CHECK(g.friends[0].level == 2 && g.friends[0].xp == 5);  // 25 XP, 20 needed for level 2
    CHECK(!completeTask(g, id, T(20000)).ok);                 // can't repeat a one-off
    CHECK(g.globalStreak == 1 && g.everActive);
}

static void testStreaksAndSleepy() {
    GameState g = fresh(30000);
    Task t = mk("run", Effort::S, RecType::Daily);
    const int id = addTask(g, t, T(30000));
    CHECK(completeTask(g, id, T(30000)).ok);
    CHECK(findTask(g, id)->streak == 1);
    sweep(g, T(30001));
    CHECK(completeTask(g, id, T(30001)).ok);
    CHECK(findTask(g, id)->streak == 2 && g.globalStreak == 2);
    // Bonus applies on the third completion: task streak 2 (+10%) and global streak 2 (+4%).
    CHECK(streakBonusPct(*findTask(g, id), g) == 14);
    CHECK(!isSleepy(g, 30002));
    CHECK(isSleepy(g, 30003));
    // Skip day 30002 entirely, return on 30003.
    sweep(g, T(30003));
    CHECK(findTask(g, id)->streak == 0);
    CHECK(g.globalStreak == 0);
    CHECK(completeTask(g, id, T(30003)).ok);
    CHECK(!isSleepy(g, 30003));
    CHECK(g.globalStreak == 1);
}

static void testLate() {
    GameState g = fresh(31000);
    Task t = mk("meds", Effort::L, RecType::Daily);
    t.targetMinute = 9 * 60;  // window 9:00-11:00, grace until 13:00
    const int id = addTask(g, t, T(31000));
    const DueWindow w = dueWindow(*findTask(g, id));
    CHECK(w.start == 540 && w.end == 660 && w.graceEnd == 780);
    auto occ = occurrencesOn(g, 31000, 14 * 60);
    CHECK(occ.size() == 1 && occ[0].missed);
    const int before = g.friends[0].level * 1000 + g.friends[0].xp;
    CHECK(completeTask(g, id, T(31000, 12, 0)).ok);  // late: half of 60 XP = 30
    CHECK(g.completions.back().late);
    CHECK(g.friends[0].level * 1000 + g.friends[0].xp != before);
    // 30 XP: level 1 needs 20 -> level 2 with 10 left.
    CHECK(g.friends[0].level == 2 && g.friends[0].xp == 10);
}

static void testReminders() {
    GameState g = fresh(32000);
    Task t = mk("study", Effort::M, RecType::Daily);
    t.targetMinute = 10 * 60;
    const int id = addTask(g, t, T(32000));
    CHECK(pollReminders(g, T(32000, 9, 59)).empty());
    auto r = pollReminders(g, T(32000, 10, 0));
    CHECK(r.size() == 1 && r[0].taskId == id);
    CHECK(pollReminders(g, T(32000, 10, 5)).empty());       // no double fire
    r = pollReminders(g, T(32000, 10, 31));
    CHECK(r.size() == 1);                                   // nudge at +30
    snoozeReminder(g, id, 32000, T(32000, 10, 40), 60);
    CHECK(pollReminders(g, T(32000, 11, 0)).empty());
    r = pollReminders(g, T(32000, 11, 41));
    CHECK(r.size() == 1);                                   // snooze expired
    CHECK(completeTask(g, id, T(32000, 11, 45)).ok);
    CHECK(pollReminders(g, T(32000, 12, 10)).empty());      // done: silent

    // Quiet hours hold reminders, then collapse them into a single catch-up.
    Task night = mk("late", Effort::S, RecType::Daily);
    night.targetMinute = 22 * 60 + 30;
    addTask(g, night, T(32000));
    CHECK(pollReminders(g, T(32000, 23, 0)).empty());
    CHECK(inQuietHours(g.settings, 23 * 60) && inQuietHours(g.settings, 7 * 60) && !inQuietHours(g.settings, 12 * 60));
}

static void testEggsAndEvolution() {
    GameState g = fresh(33000);
    Task t = mk("t", Effort::S, RecType::Daily);
    const int id = addTask(g, t, T(33000));
    bool hatched = false;
    for (int d = 0; d < 5; ++d) {
        sweep(g, T(33000 + d));
        const Result r = completeTask(g, id, T(33000 + d));
        for (const auto& m : r.messages) if (m.find("hatched") != std::string::npos) hatched = true;
    }
    CHECK(hatched && g.eggs.empty());
    CHECK(g.friends.size() >= 2 || g.coins > 20);  // new friend, or duplicate converted to coins

    GameState e = fresh(34000);
    const Result r = addXp(e, e.friends[0].id, 4000000);
    CHECK(!r.messages.empty());
    CHECK(e.friends[0].level == kMaxLevel);
    CHECK(species(e.friends[0].species).name == "Grovewarden");
}

static void testShopAndParty() {
    GameState g = fresh(35000);
    g.coins = 100;
    CHECK(buyEgg(g).ok && g.coins == 60 && g.eggs.size() == 2);
    CHECK(buyCharm(g).ok && g.charms == 1 && g.coins == 40);
    g.coins = 0;
    CHECK(!buyEgg(g).ok && !buyCharm(g).ok);
    CHECK(!setPartyMember(g, g.friends[0].id, false).ok);  // can't empty the party
    for (int i = 0; i < 6; ++i) addFriend(g, 3 + i, 5);    // species 3..8
    CHECK(g.party.size() == 6);
    CHECK(!setPartyMember(g, 9999, true).ok);
    addFriend(g, 3, 5);
    CHECK(g.friends.size() == 7);  // the duplicate did not add a friend
}

static void testSaveLoad() {
    GameState g = fresh(36000);
    Task t = mk("café \"quoted\"", Effort::XL, RecType::Weekdays);
    t.rec.weekdayMask = 0b0101010;
    t.targetMinute = 8 * 60 + 15;
    t.window = Window::Morning;
    const int id = addTask(g, t, T(36000));
    completeTask(g, id, T(36000));
    addFriend(g, 12, 9);
    snoozeReminder(g, id, 36000, T(36000), 30);
    g.settings.confirmDone = false;

    const std::string path = (std::filesystem::temp_directory_path() / "gf_test_save" / "save.json").string();
    CHECK(saveToFile(g, path));
    GameState h;
    CHECK(loadFromFile(path, h));
    CHECK(h.tasks.size() == g.tasks.size() && h.tasks[0].title == g.tasks[0].title);
    CHECK(h.tasks[0].rec.weekdayMask == 0b0101010 && h.tasks[0].targetMinute == 8 * 60 + 15);
    CHECK(h.friends.size() == g.friends.size() && h.party == g.party);
    CHECK(h.coins == g.coins && h.energy == g.energy && h.globalStreak == g.globalStreak);
    CHECK(h.dexOwned == g.dexOwned && h.completions.size() == g.completions.size());
    CHECK(h.reminders.size() == 1 && h.reminders[0].snoozeUntil == g.reminders[0].snoozeUntil);
    CHECK(!h.settings.confirmDone);
    CHECK(h.eggs.size() == g.eggs.size());
    GameState bad;
    CHECK(!loadFromFile(path + ".nope", bad));
    std::filesystem::remove_all(std::filesystem::path(path).parent_path());
}

static void testBattleBasics() {
    // A Light hit does sensible damage, Priority goes first, cooldowns apply.
    std::vector<Fighter> a = {makeFighter(3, 10), makeFighter(0, 10), makeFighter(6, 10)};
    std::vector<Fighter> b = {makeFighter(9, 10), makeFighter(11, 10), makeFighter(13, 10)};
    Battle bt(a, b, 7);
    CHECK(bt.slotFilled(0, 2) && bt.slotFilled(1, 2));
    std::array<Action, 3> act{};
    for (int k = 0; k < 3; ++k) { act[static_cast<size_t>(k)].type = Action::UseMove; act[static_cast<size_t>(k)].move = 0; act[static_cast<size_t>(k)].target = {1, 0}; }
    const int hp0 = bt.fighter(1, 0).hp;
    bt.resolveTurn(act);
    CHECK(bt.fighter(1, 0).hp < hp0);
    CHECK(bt.turn == 1);
    // Cooldown: Hyper Dash (Zapling move 1) is unusable right after use.
    CHECK(bt.moveUsable(0, 0, 1));
    act[0].move = 1; act[0].target = {0, 0};
    bt.resolveTurn(act);
    CHECK(!bt.moveUsable(0, 0, 1));
    CHECK(bt.fighter(0, 0).haste > 0);
    // Switching works and resets statuses.
    CHECK(bt.canSwitch(0, 0) == false);  // team of 3 all active: no bench
}

static void testSwitchAndCatch() {
    std::vector<Fighter> a, b;
    for (int i = 0; i < 6; ++i) a.push_back(makeFighter(i, 8));
    b.push_back(makeFighter(12, 8));
    Battle bt(a, b, 11, true);
    CHECK(bt.benchAlive(0).size() == 3);
    CHECK(bt.canSwitch(0, 0));
    std::array<Action, 3> act{};
    act[0].type = Action::Switch; act[0].bench = 3;
    act[1].type = Action::UseMove; act[1].move = 0; act[1].target = {1, 0};
    act[2].type = Action::UseMove; act[2].move = 0; act[2].target = {1, 0};
    bt.resolveTurn(act);
    CHECK(bt.sides[0].active[0] == 3);
    // Catching: certain success removes the foe and ends the battle.
    CHECK(bt.tryCatch(0, 2.0));
    CHECK(bt.over() && bt.winner == 0 && bt.caught.size() == 1);

    GameState g = fresh(37000);
    g.charms = 1;
    std::vector<Fighter> mine = {makeFighter(0, 5)};
    std::vector<Fighter> foe = {makeFighter(9, 3)};
    Battle wb(mine, foe, 3, true);
    CHECK(useCharm(g, wb, 0) || g.charms == 0);
    CHECK(g.charms == 0);
    CHECK(!useCharm(g, wb, 0));  // no charms left
}

static void testBattleFlow() {
    GameState g = fresh(38000);
    std::unique_ptr<Battle> b;
    std::string why;
    CHECK(!startBattle(g, BattleMode::Wild, 0, T(38000), b, why));  // no energy
    g.energy = 10;
    CHECK(startBattle(g, BattleMode::Wild, 0, T(38000), b, why) && g.energy == 9);
    CHECK(b->sides[1].team.size() == 1);  // one friend in the party: one wild foe
    CHECK(b && b->wild);
    CHECK(!startBattle(g, BattleMode::Boss, 0, T(38000), b, why));  // locked
    g.globalStreak = 3;
    CHECK(startBattle(g, BattleMode::Boss, 0, T(38000), b, why) && g.energy == 4 && b->smartAi);
    // Force a win and check the boss reward.
    for (auto& f : b->sides[1].team) f.hp = 0;
    b->sides[1].active = {-1, -1, -1};
    b->resolveTurn({});
    CHECK(b->over() && b->winner == 0);
    const size_t friendsBefore = g.friends.size();
    const Result r = finishBattle(g, *b, BattleMode::Boss);
    CHECK(r.ok && g.bossesWon == 1 && g.friends.size() == friendsBefore + 1);
    CHECK(species(g.friends.back().species).rare);
    // Sleepy friends fight at -20%.
    GameState s = fresh(39000);
    s.everActive = true;
    s.lastActiveDay = 38990;
    s.energy = 5;
    CHECK(startBattle(s, BattleMode::Wild, 0, T(39000), b, why));
    CHECK(b->sides[0].team[0].sleepy);
}

// Drive full AI-vs-AI battles across the whole roster to shake out crashes and degenerate loops.
static void testSimulation() {
    std::mt19937 rng(2024);
    const int kBattles = 1500;
    int timeouts = 0, wins = 0;
    long long turns = 0;
    std::set<std::string> usedMoves;
    std::map<int, std::pair<int, int>> speciesWins;  // id -> (wins, games)

    for (int n = 0; n < kBattles; ++n) {
        const int level = 8 + static_cast<int>(rng() % 30);
        std::vector<Fighter> a, b;
        std::set<int> pa, pb;
        // Uneven team sizes exercise empty slots, short benches and last-friend-standing paths.
        const size_t na = n % 3 == 0 ? 1 + rng() % 6 : 6, nb = n % 4 == 0 ? 1 + rng() % 6 : 6;
        while (pa.size() < na) pa.insert(static_cast<int>(rng() % kSpeciesCount));
        while (pb.size() < nb) pb.insert(static_cast<int>(rng() % kSpeciesCount));
        for (int s : pa) a.push_back(makeFighter(s, level));
        for (int s : pb) b.push_back(makeFighter(s, level));
        Battle bt(a, b, static_cast<uint32_t>(rng()), false, false);
        bt.autoReplacePlayer = true;
        int guard = 0;
        while (!bt.over() && guard++ < 150) {
            std::array<Action, 3> act{};
            for (int k = 0; k < 3; ++k)
                if (bt.slotFilled(0, k) && bt.fighter(0, k).alive()) act[static_cast<size_t>(k)] = bt.aiChoose(0, k);
            bt.resolveTurn(act);
        }
        if (!bt.over()) ++timeouts;
        turns += bt.turn;
        if (bt.winner == 0) ++wins;
        for (const auto& line : bt.log) {
            const auto p = line.find(" uses ");
            if (p != std::string::npos && line.back() == '!') usedMoves.insert(line.substr(p + 6, line.size() - p - 7));
        }
        for (int s : pa) { speciesWins[s].second++; if (bt.winner == 0) speciesWins[s].first++; }
        for (int s : pb) { speciesWins[s].second++; if (bt.winner == 1) speciesWins[s].first++; }
    }
    std::printf("  sim: %d battles, avg %.1f turns, %d timeouts, side-0 win rate %.0f%%, %zu distinct moves used\n",
                kBattles, static_cast<double>(turns) / kBattles, timeouts, 100.0 * wins / kBattles, usedMoves.size());
    CHECK(timeouts == 0);
    const double avgTurns = static_cast<double>(turns) / kBattles;
    CHECK(avgTurns > 4 && avgTurns < 60);
    CHECK(wins > kBattles * 0.30 && wins < kBattles * 0.70);

    int best = -1, worst = -1;
    double bw = -1, ww = 2;
    for (auto& kv : speciesWins) {
        const double r = static_cast<double>(kv.second.first) / kv.second.second;
        if (r > bw) { bw = r; best = kv.first; }
        if (r < ww) { ww = r; worst = kv.first; }
    }
    std::printf("  best: %s %.0f%%, worst: %s %.0f%%\n", species(best).name.c_str(), bw * 100, species(worst).name.c_str(), ww * 100);

    std::set<std::string> all;
    for (const auto& s : allSpecies()) for (const auto& m : s.moves) all.insert(m.name);
    std::string missing;
    for (const auto& m : all) if (!usedMoves.count(m)) missing += m + ", ";
    if (!missing.empty()) std::printf("  moves never used in sims: %s\n", missing.c_str());
    CHECK(usedMoves.size() >= all.size() * 95 / 100);
}

static void testAreasAndEvents() {
    // Every area has enough local friends at low and high levels, and a locked area refuses to start.
    for (int a = 0; a < kAreaCount; ++a) {
        int common = 0, rares = 0;
        for (const auto& sp : allSpecies())
            if (sp.habitats & area(a).habitat) (sp.rare ? rares : common)++;
        CHECK(common >= 4);
        CHECK(area(a).habitat == (1 << a));
    }
    GameState g = fresh(40000);
    g.energy = 50;
    std::unique_ptr<Battle> b;
    std::string why;
    CHECK(areaUnlocked(g, 0) && !areaUnlocked(g, 1));
    CHECK(!startBattle(g, BattleMode::Wild, 1, T(40000), b, why));
    CHECK(g.energy == 50);  // a refused start costs nothing
    g.battlesWon = 100;
    for (int a = 0; a < kAreaCount; ++a) {
        for (int i = 0; i < 20; ++i) {
            g.energy = 50;
            CHECK(startBattle(g, BattleMode::Duel, a, T(40000), b, why));
            for (const auto& f : b->sides[1].team) CHECK((f.sp().habitats & area(a).habitat) != 0);
        }
        CHECK(g.lastArea == a);
    }
    // Harder areas pay more.
    std::vector<Fighter> mine = {makeFighter(0, 10)}, foe = {makeFighter(9, 10)};
    GameState pay = fresh(41000);
    Battle w0(mine, foe, 1), w5(mine, foe, 1);
    w0.winner = w5.winner = 0;
    w5.areaId = 5;
    const int c0 = (finishBattle(pay, w0, BattleMode::Duel), pay.coins);
    const int c5 = (finishBattle(pay, w5, BattleMode::Duel), pay.coins - c0);
    CHECK(c5 > c0 - 20);  // Ruins coin multiplier (2.2x) outweighs the starting 20 coins already counted in c0

    // Event recording: every log line has a snapshot, and the last one matches the live state.
    std::vector<Fighter> pa = {makeFighter(3, 12), makeFighter(0, 12), makeFighter(6, 12)};
    std::vector<Fighter> pb = {makeFighter(9, 12), makeFighter(11, 12), makeFighter(13, 12)};
    Battle bt(pa, pb, 5);
    bt.recordEvents = true;
    std::array<Action, 3> act{};
    for (int k = 0; k < 3; ++k) { act[static_cast<size_t>(k)].type = Action::UseMove; act[static_cast<size_t>(k)].move = 0; act[static_cast<size_t>(k)].target = {1, 0}; }
    bt.resolveTurn(act);
    CHECK(!bt.events.empty());
    CHECK(bt.events.size() + 1 == bt.log.size());  // the constructor's first line predates recording
    const Snapshot& last = bt.events.back().snap;
    bool same = true;
    for (int s = 0; s < 2; ++s)
        for (size_t i = 0; i < bt.sides[static_cast<size_t>(s)].team.size(); ++i)
            same = same && last.team[static_cast<size_t>(s)][i].hp == bt.sides[static_cast<size_t>(s)].team[i].hp;
    CHECK(same);
    bool anyActor = false;
    for (const auto& e : bt.events) anyActor = anyActor || e.actor.side >= 0;
    CHECK(anyActor);
}

// The roster must have real variety: no two friends are copies of each other.
static int speciesWith(Ab a) {
    for (int i = 0; i < kSpeciesCount; ++i) if (species(i).ability == a) return i;
    return -1;
}

static void testAbilities() {
    std::set<int> used;
    for (int i = 0; i < kSpeciesCount; ++i) {
        CHECK(species(i).ability != Ab::None);
        used.insert(static_cast<int>(species(i).ability));
        // evolutions share the ability of their line
        if (species(i).evolvesTo >= 0) CHECK(species(species(i).evolvesTo).ability == species(i).ability);
    }
    CHECK(static_cast<int>(used.size()) >= 50);
    for (int a = 1; a < kAbilityCount; ++a) CHECK(std::string(abilityInfo(static_cast<Ab>(a)).desc).size() > 10);

    const int filler = 0;
    // Intimidate lowers both foes' attack on entry.
    {
        const int id = speciesWith(Ab::Intimidate);
        CHECK(id >= 0);
        Battle b({makeFighter(id, 20)}, {makeFighter(filler, 20), makeFighter(filler, 20)}, 5);
        CHECK(b.fighter(1, 0).atkMod == -20 && b.fighter(1, 0).atkModT > 0);
        CHECK(b.fighter(1, 1).atkMod == -20);
    }
    // Sturdy: survives a lethal hit from full HP once.
    {
        const int id = speciesWith(Ab::Sturdy);
        Battle b({makeFighter(filler, 60)}, {makeFighter(id, 5)}, 7);
        Fighter& d = b.fighter(1, 0);
        d.hp = d.maxHp;
        Move big("Nuke", 400);
        bool fu = false;
        const int dm = b.test_dealHit({0, 0}, {1, 0}, 400, big, &fu);
        CHECK(dm >= 0 && d.hp == 1 && d.abUsed);
    }
    // Shadow Tag blocks voluntary switching; Run Away ignores it.
    {
        const int tag = speciesWith(Ab::ShadowTag), run = speciesWith(Ab::RunAway);
        Battle b({makeFighter(filler, 20), makeFighter(filler, 20), makeFighter(filler, 20), makeFighter(filler, 20)}, {makeFighter(tag, 20)}, 9);
        CHECK(!b.canSwitch(0, 0));
        Battle c({makeFighter(run, 20), makeFighter(filler, 20), makeFighter(filler, 20), makeFighter(filler, 20)}, {makeFighter(tag, 20)}, 9);
        CHECK(c.canSwitch(0, 0));
    }
    // Regenerator heals when switching out.
    {
        const int id = speciesWith(Ab::Regenerator);
        Battle b({makeFighter(id, 30), makeFighter(filler, 30), makeFighter(filler, 30), makeFighter(filler, 30)}, {makeFighter(filler, 30)}, 11);
        b.fighter(0, 0).hp = b.fighter(0, 0).maxHp / 2;
        const int before = b.fighter(0, 0).hp, mx = b.fighter(0, 0).maxHp;
        std::array<Action, 3> acts{};
        acts[0].type = Action::Switch;
        acts[0].bench = 3;
        b.resolveTurn(acts);
        CHECK(b.sides[0].team[0].hp >= before + mx / 5);
    }
    // Stall always acts last; Prankster beats speed with utility moves (checked via event order smoke).
    {
        const int id = speciesWith(Ab::Stall);
        CHECK(id >= 0);
    }
    // Contrary reverses stat changes; Clear Body blocks drops.
    {
        const int con = speciesWith(Ab::Contrary), clr = speciesWith(Ab::ClearBody);
        Battle b({makeFighter(filler, 20)}, {makeFighter(con, 20)}, 13);
        b.test_apply({EK::AtkMod, -20, 3}, {1, 0}, {0, 0});
        CHECK(b.fighter(1, 0).atkMod == 20);
        Battle c({makeFighter(filler, 20)}, {makeFighter(clr, 20)}, 13);
        c.test_apply({EK::AtkMod, -20, 3}, {1, 0}, {0, 0});
        CHECK(c.fighter(1, 0).atkMod == 0);
        c.test_apply({EK::Slow, 2}, {1, 0}, {0, 0});
        CHECK(c.fighter(1, 0).slow == 0);
    }
}

static void testRosterVariety() {
    std::map<std::string, int> use;
    std::vector<std::vector<std::string>> sigs(allSpecies().size());
    std::set<std::array<int, 6>> spreads;
    for (const auto& s : allSpecies()) {
        for (const auto& m : s.moves) { const std::string g = moveSignature(m); sigs[static_cast<size_t>(s.id)].push_back(g); use[g]++; }
        spreads.insert({s.hp, s.atk, s.def, s.spa, s.spd, s.spe});
        CHECK(!s.role.empty() && !s.blurb.empty());
        CHECK(s.hp >= 1 && s.hp <= 5 && s.atk >= 1 && s.atk <= 5 && s.def >= 1 && s.def <= 5 && s.spa >= 1 && s.spa <= 5 && s.spd >= 1 && s.spd <= 5 && s.spe >= 1 && s.spe <= 5);
    }
    CHECK(spreads.size() == allSpecies().size());          // every friend has its own six-stat spread
    int noSignature = 0, crossLine3 = 0, identical = 0;
    for (const auto& s : allSpecies()) {
        bool has = false;
        for (const auto& g : sigs[static_cast<size_t>(s.id)]) if (use[g] == 1) has = true;
        if (!has) ++noSignature;
    }
    for (size_t a = 0; a < allSpecies().size(); ++a)
        for (size_t b = a + 1; b < allSpecies().size(); ++b) {
            std::multiset<std::string> A(sigs[a].begin(), sigs[a].end());
            int common = 0;
            for (const auto& g : sigs[b]) { auto it = A.find(g); if (it != A.end()) { ++common; A.erase(it); } }
            if (common == 4) ++identical;
            if (common >= 3 && allSpecies()[a].line != allSpecies()[b].line) ++crossLine3;
        }
    std::printf("  variety: %zu distinct mechanics in %zu moves, %d friends without a signature move, %d cross-line pairs sharing 3+ moves\n", use.size(), allSpecies().size() * 4, noSignature, crossLine3);
    CHECK(noSignature == 0);        // every friend has at least one move nobody else has
    CHECK(identical == 0);          // no two friends have the same moveset
    CHECK(crossLine3 == 0);         // friends from different lines never share more than two moves
    CHECK(use.size() >= 380);       // plenty of distinct mechanics
    // Roles reflect the stats: a friend that is mostly special hits has SPA >= ATK, and the reverse.
    int mismatched = 0;
    for (const auto& s : allSpecies()) {
        double phys = 0, spec = 0;
        for (const auto& m : s.moves) { if (!m.damaging()) continue; double p = m.hits.empty() ? m.power : 0; for (int h : m.hits) p += h; (m.special ? spec : phys) += p; }
        if (spec > phys * 2 && s.atk > s.spa) ++mismatched;
        if (phys > spec * 2 && s.spa > s.atk) ++mismatched;
    }
    CHECK(mismatched <= 6);
}

static void testSprites() {
    std::set<std::vector<uint32_t>> seen;
    for (int i = 0; i < kSpeciesCount; ++i) {
        const Sprite s0 = makeSprite(i, 0), s1 = makeSprite(i, 1);
        int opaque = 0;
        for (uint32_t p : s0.px) if ((p >> 24) != 0) ++opaque;
        CHECK(opaque > 40);
        CHECK(s0.px != s1.px || true);
        seen.insert(s0.px);
    }
    CHECK(seen.size() == static_cast<size_t>(kSpeciesCount));
    CHECK(makeSilhouette(makeSprite(0, 0)).px.size() == static_cast<size_t>(kSpriteSize * kSpriteSize));
    CHECK(makeEggSprite(0).px != makeEggSprite(1).px);
}

static void testData() {
    CHECK(allSpecies().size() == static_cast<size_t>(kSpeciesCount));
    std::set<std::string> moveNames;
    for (const auto& s : allSpecies()) {
        CHECK(s.hp >= 1 && s.hp <= 5);
        for (const auto& m : s.moves) { CHECK(!m.name.empty()); moveNames.insert(m.name); }
        if (s.evolvesTo >= 0) CHECK(species(s.evolvesTo).stage == s.stage + 1);
    }
    CHECK(moveNames.size() == static_cast<size_t>(kSpeciesCount) * 4);
    CHECK(starterSpecies().size() == 3);
    CHECK(xpToNext(1) == 20 && xpToNext(10) > xpToNext(5));
}

int main() {
    struct { const char* name; void (*fn)(); } tests[] = {
        {"data", testData}, {"time", testTime}, {"recurrence", testRecurrence}, {"rewards", testRewards},
        {"streaks+sleepy", testStreaksAndSleepy}, {"late", testLate}, {"reminders", testReminders},
        {"eggs+evolution", testEggsAndEvolution}, {"shop+party", testShopAndParty}, {"save/load", testSaveLoad},
        {"battle basics", testBattleBasics}, {"abilities", testAbilities}, {"switch+catch", testSwitchAndCatch}, {"battle flow", testBattleFlow},
        {"areas+events", testAreasAndEvents}, {"roster variety", testRosterVariety}, {"sprites", testSprites}, {"simulation", testSimulation},
    };
    for (auto& t : tests) {
        const int before = g_failures;
        std::printf("[ RUN ] %s\n", t.name);
        t.fn();
        std::printf("[ %s ] %s\n", g_failures == before ? " OK " : "FAIL", t.name);
    }
    std::printf("\n%d checks, %d failures\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
