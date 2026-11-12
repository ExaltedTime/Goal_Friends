// Immediate-mode UI for the desktop strip: world, panels and the battle screen.
#pragma once
#include <memory>
#include <string>
#include <vector>

#include "game.hpp"
#include "gfx.hpp"

namespace ui {

struct Input {
    float mx = -1, my = -1;  // logical coordinates, -1 when the mouse is outside the window
    bool down = false, pressed = false;
    float wheel = 0;
    std::wstring typed;
    std::wstring paste;
    bool backspace = false, enter = false, escape = false;
};

enum class Tab { None, Start, Tasks, Friends, Dex, Shop, Focus, Battle, Guide };

// A friend walking on the strip. Position is in ground cells (one cell = one sprite pixel) and moves in whole-cell steps
// locked to the frame, so the motion is perfectly even instead of jittering between 2 and 6 pixel jumps.
struct Walker {
    int friendId = 0;
    int cx = 60;            // horizontal centre, in cells
    int dir = 1;            // +1 right, -1 left
    bool fast = false;      // fast walkers step every frame, slow ones every other frame
    bool phase = false;
    bool init = false;
    float walkT = 3, pause = 0;
    float cps = 10;        // walking speed in cells per second (each friend has its own)
    float stepAcc = 0;     // fractional cells carried between frames
    float animT = 0;       // this friend's own animation clock (never shared with the others)
    float animRate = 4;    // walk-cycle frames per second
    float idleT = 0, idleRate = 1.5f;
    float moveT = 0;       // how long since the last step (so a stopped friend settles into its idle pose)
    float hop = 0, hopV = 0;
    unsigned rng = 1;
};

class Ui {
public:
    explicit Ui(gf::GameState& g);

    // One frame: handle input, advance animation, draw. W/H are logical pixels.
    void frame(gfx::Gfx& gx, float W, float H, Input& in, double dt, long long nowMs, tu::Time now);

    float desiredHeight() const;
    bool panelOpen() const { return tab_ != Tab::None; }
    bool wantsKeyboard() const { return tab_ != Tab::None; }
    bool takeDirty() { const bool d = dirty_; dirty_ = false; return d; }
    std::vector<std::string> takeBalloons();
    void toast(const std::string& s);
    void balloon(const std::string& s) { balloons_.push_back(s); }
    void openTab(Tab t) { tab_ = t; }
    // Called by the host when new reminders fire so the strip can nag too.
    void reminderFired() { nagSeconds_ = 20; }
    // True while something on screen is moving (friends walking, toasts, battle effects); lets the host pick a frame rate.
    bool animating() const { return animating_; }
    // True during battle replays, floating numbers and toasts: things that need smoother frames than walking friends.
    bool effectsActive() const { return playing_ || !floats_.empty() || !toasts_.empty(); }
    // Cheap work that must keep running while the window is hidden (a focus session ending).
    void backgroundTick(long long nowMs, tu::Time now);
    bool locked() const { return focusRunning_; }  // during a focus session everything except the Focus tab is off limits

    static constexpr float kStrip = 150.f;
    static constexpr float kPanelH = 520.f;

private:
    struct Toast { std::string text; float life; };
    struct Rect { float x, y, w, h; };

    // widgets
    bool hover(float x, float y, float w, float h) const;
    bool button(float x, float y, float w, float h, const std::string& label, gfx::Color col, bool enabled = true, float fs = 13, bool modal = false);
    bool textField(int id, float x, float y, float w, float h, std::string& s, const std::string& placeholder);
    bool checkbox(float x, float y, const std::string& label, bool value, bool& changed);
    void card(float x, float y, float w, float h, gfx::Color c = {0.13f, 0.16f, 0.23f, 1.f});
    void bar(float x, float y, float w, float h, double frac, gfx::Color c);

    // screens
    void drawWorld(float dt);
    void drawPanel();
    void drawStart(const Rect& b);
    void drawTasks(const Rect& b);
    void drawFriends(const Rect& b);
    void drawDex(const Rect& b);
    void drawShop(const Rect& b);
    void drawFocus(const Rect& b);
    void drawBattle(const Rect& b);
    void drawGuide(const Rect& b);
    void drawGround(float gy, float gh);
    void paintGround(float gy, float gh);
    void refreshDue();

    // task helpers
    void submitTaskForm();
    void completeNow(int taskId);
    std::string scheduleText(const gf::Task& t) const;
    std::string timeText(const gf::Task& t) const;

    // battle helpers
    void beginBattle(gf::BattleMode m);
    void battleAdvance();
    void battleResolve();
    void battleFinish(bool ran);
    void fighterCard(float x, float y, float w, float h, int side, int slot, bool highlight, bool targetable, bool* clicked);
    void battleAfterTurn();
    void stepPlayback();
    void spawnEffects(const gf::Snapshot& before, const gf::Snapshot& after, gf::Ref actor);
    const gf::Fighter* shownFighter(int side, int slot, int* teamIdx) const;
    int shownBench(int side) const;
    Rect cardRect(int side, int slot) const;
    void showResultLines(const std::vector<std::string>& lines);

    gf::GameState& g_;
    gfx::Gfx* gx_ = nullptr;
    Input* in_ = nullptr;
    float W_ = 0, H_ = 0;
    long long nowMs_ = 0;
    tu::Time now_{};
    bool consumed_ = false;
    bool blocked_ = false;
    bool dirty_ = false;
    int focusField_ = -1;
    bool fieldClicked_ = false;

    Tab tab_ = Tab::None;
    Rect panel_{};
    std::vector<Toast> toasts_;
    std::vector<std::string> balloons_;
    std::vector<Walker> walkers_;
    float animT_ = 0;
    float dt_ = 0;
    bool groupWalk_ = true;      // the whole party alternates between a stroll and a shared rest, so the strip is often idle
    float groupT_ = 4.f;
    unsigned groupRng_ = 7;
    bool animating_ = false;
    int dueTab_ = 0, dueNow_ = 0;   // cached once a second: tasks still to do today / already due
    float dueTimer_ = 0;
    float nagSeconds_ = 0;

    // tasks tab
    int taskView_ = 0;  // 0 today, 1 all
    float scrollToday_ = 0, scrollAll_ = 0;
    int confirmTask_ = -1;
    std::string formTitle_;
    int formEffort_ = 1, formRepeat_ = 0, formWindow_ = 0, formHour_ = 9, formMin_ = 0, formN_ = 2;
    int formDays_ = 0b0111110;
    bool formRemind_ = true;

    // friends / dex
    int selFriend_ = -1, selDex_ = -1;
    float scrollFriends_ = 0, scrollDex_ = 0;
    float taskContent_[2] = {0, 0};   // content heights from the previous frame, so scrolling is clamped before drawing
    float guideContent_ = 0;

    // focus
    bool focusRunning_ = false;
    long long focusEndMs_ = 0;
    int focusMinutes_ = 25;
    int focusDone_ = 0;  // minutes of the session that just finished (0 = none)
    bool battleReady_ = false;  // a focus session ended: the Battle button pulses until you open it

    // guide
    int guideSection_ = 0;
    float scrollGuide_ = 0;

    // battle
    std::unique_ptr<gf::Battle> battle_;
    gf::BattleMode mode_ = gf::BattleMode::Wild;
    enum class Phase { Choose, Target, CharmTarget, SwitchPick, Replace, Done } phase_ = Phase::Choose;
    std::array<gf::Action, 3> acts_{};
    std::array<bool, 3> chosen_{{false, false, false}};
    int curSlot_ = 0;
    int pendingMove_ = -1;
    size_t logShown_ = 0;
    std::vector<std::string> resultLines_;
    std::string battleNote_;

    // turn playback: the engine resolves a whole turn at once, the UI replays its events one at a time
    struct FloatText { float x, y; std::string text; gfx::Color col; float life; };
    std::vector<FloatText> floats_;
    bool playing_ = false;
    bool fastPlay_ = false;
    size_t playStart_ = 0, playEnd_ = 0;
    int viewRel_ = -1;       // -1 = the state before the turn, else index into this turn's events
    float playTimer_ = 0;
    gf::Snapshot prevSnap_;
    float hpAnim_[2][6] = {{-1, -1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1, -1}};
    float flash_[2][3] = {{0, 0, 0}, {0, 0, 0}};
    float glow_[2][3] = {{0, 0, 0}, {0, 0, 0}};
    float flashGood_[2][3] = {{0, 0, 0}, {0, 0, 0}};
    Rect battleBody_{};
};

}  // namespace ui
