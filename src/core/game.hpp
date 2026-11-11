// Game state and rules: tasks, recurrence, rewards, streaks, reminders, eggs, shop, battles setup, saving.
// Pure logic with an injected clock, so everything here is unit-testable on any OS.
#pragma once
#include <cstdint>
#include <memory>
#include <random>
#include <set>
#include <string>
#include <vector>

#include "battle.hpp"
#include "data.hpp"
#include "json.hpp"
#include "timeutil.hpp"

namespace gf {

enum class Effort { S, M, L, XL };
int baseXp(Effort e);
int energyFor(Effort e);
int coinsFor(Effort e);
const char* effortName(Effort e);

enum class RecType { Once, Daily, Weekdays, EveryN, Weekly, Monthly };
enum class Window { Any, Morning, Afternoon, Evening };

struct Recurrence {
    RecType type = RecType::Once;
    int weekdayMask = 0;  // bit 0 = Sunday ... bit 6 = Saturday (Weekdays)
    int n = 1;            // EveryN
    int weekday = 1;      // Weekly (0 = Sunday)
    int monthday = 1;     // Monthly (0 = last day of month)
    int anchorDay = 0;    // EveryN counts from here
};

struct Task {
    int id = 0;
    std::string title;
    Effort effort = Effort::M;
    Recurrence rec;
    std::string category;
    int assignedTo = -1;   // friend id, or -1 for the party lead
    int targetMinute = -1; // minute of day, or -1
    Window window = Window::Any;
    bool remind = true;
    bool paused = false;
    bool archived = false; // finished one-off or deleted
    int streak = 0;
    int createdDay = 0;
    int dueDay = 0;        // Once
};

struct DueWindow { int start, end, graceEnd; };  // minute of day
DueWindow dueWindow(const Task& t);
bool occursOn(const Task& t, int day);

struct Completion { int taskId = 0; int day = 0; int minute = 0; bool late = false; };

struct Friend { int id = 0; int species = 0; int level = 1; int xp = 0; };
struct Egg { int progress = 0; int needed = 5; };

struct ReminderState { int taskId = 0; int day = 0; int stage = 0; long long snoozeUntil = 0; bool dismissed = false; };

struct Settings {
    bool confirmDone = true;
    int quietStart = 22 * 60;
    int quietEnd = 8 * 60;
};

struct GameState {
    int version = 1;
    std::vector<Task> tasks;
    std::vector<Completion> completions;
    std::vector<Friend> friends;
    std::vector<int> party;  // friend ids, max 6; party[0] is the lead
    std::vector<Egg> eggs;
    int energy = 0, coins = 0, charms = 0;
    int globalStreak = 0, lastActiveDay = -1000000;
    bool everActive = false;
    int nextTaskId = 1, nextFriendId = 1;
    std::set<int> dexOwned, dexSeen;
    bool started = false;  // starter chosen
    int lastSweepDay = -1000000;
    int totalTasksDone = 0;
    int bossesWon = 0, battlesWon = 0, battlesPlayed = 0;
    int bossUnlockedUntilWins = 0;
    std::vector<ReminderState> reminders;
    Settings settings;
    uint32_t rngSeed = 12345;
};

// Occurrence of a task on a given day.
struct Occurrence {
    int taskId = 0;
    int day = 0;
    bool done = false;
    bool late = false;
    bool missed = false;  // finished window and grace without being done (recurring only)
    bool overdue = false; // one-off whose due day has passed
    DueWindow win{0, 1439, 1439};
};

std::vector<Occurrence> occurrencesOn(const GameState& g, int day, int nowMinute);
const Task* findTask(const GameState& g, int id);
Task* findTask(GameState& g, int id);
Friend* findFriend(GameState& g, int id);
const Friend* findFriend(const GameState& g, int id);

// ---- Actions ----
struct Result {
    bool ok = false;
    std::vector<std::string> messages;
};

Result chooseStarter(GameState& g, int speciesId, tu::Time now);
int addTask(GameState& g, Task t, tu::Time now);
void removeTask(GameState& g, int taskId);
Result completeTask(GameState& g, int taskId, tu::Time now);
// Run once at launch and whenever the day rolls over: ends streaks of missed recurring tasks, updates sleepy.
void sweep(GameState& g, tu::Time now);
bool isSleepy(const GameState& g, int today);
Result addXp(GameState& g, int friendId, int xp);
int streakBonusPct(const Task& t, const GameState& g);

// ---- Shop and eggs ----
constexpr int kEggCost = 40;
constexpr int kCharmCost = 20;
Result buyEgg(GameState& g);
Result buyCharm(GameState& g);
Result addFriend(GameState& g, int speciesId, int level);  // duplicates become coins
Result setPartyMember(GameState& g, int friendId, bool inParty);
int leadFriendId(const GameState& g);

// ---- Focus sessions ----
Result completeFocus(GameState& g, int minutes, tu::Time now);

// ---- Reminders ----
struct ReminderMsg { int taskId; std::string text; };
std::vector<ReminderMsg> pollReminders(GameState& g, tu::Time now);
void snoozeReminder(GameState& g, int taskId, int day, tu::Time now, int minutes);
void dismissReminder(GameState& g, int taskId, int day);
bool inQuietHours(const Settings& s, int minute);

// ---- Battles ----
enum class BattleMode { Wild, Duel, Boss };
int energyCost(BattleMode m);
bool bossUnlocked(const GameState& g);
// Returns false (with a message) if the player can't afford it or has no friends. Spends the Energy.
bool startBattle(GameState& g, BattleMode mode, tu::Time now, std::unique_ptr<Battle>& out, std::string& why);
// Apply rewards after a finished battle. Caught friends in `b.caught` are added too.
Result finishBattle(GameState& g, const Battle& b, BattleMode mode);
bool useCharm(GameState& g, Battle& b, int foeSlot);
double catchChance(const Fighter& foe);

// ---- Save / load ----
json::Value toJson(const GameState& g);
bool fromJson(const json::Value& v, GameState& g);
bool saveToFile(const GameState& g, const std::string& path);
bool loadFromFile(const std::string& path, GameState& g);

}  // namespace gf
