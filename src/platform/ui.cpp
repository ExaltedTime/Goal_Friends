#include "describe.hpp"
#include "ui.hpp"

#include <windows.h>

#include <algorithm>
#include <cmath>
#include <cstdio>

using gfx::rgba;
using gfx::TF_BOLD;
using gfx::TF_CENTER;
using gfx::TF_LEFT;
using gfx::TF_RIGHT;
using gfx::TF_VCENTER;
using gfx::TF_WRAP;

namespace ui {
namespace {

using gf::describeMove;

const gfx::Color kBg = rgba(20, 24, 36, 244);
const gfx::Color kCard = rgba(34, 40, 58);
const gfx::Color kCard2 = rgba(46, 54, 78);
const gfx::Color kText = rgba(235, 238, 245);
const gfx::Color kDim = rgba(150, 158, 176);
const gfx::Color kAccent = rgba(255, 196, 64);
const gfx::Color kGreen = rgba(70, 170, 100);
const gfx::Color kRed = rgba(200, 72, 72);
const gfx::Color kBlue = rgba(70, 120, 210);
const gfx::Color kBtn = rgba(58, 70, 104);

std::string narrow(const std::wstring& w) {
    if (w.empty()) return {};
    const int n = WideCharToMultiByte(CP_UTF8, 0, w.data(), static_cast<int>(w.size()), nullptr, 0, nullptr, nullptr);
    std::string s(static_cast<size_t>(n), '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.data(), static_cast<int>(w.size()), s.data(), n, nullptr, nullptr);
    return s;
}

void popUtf8(std::string& s) {
    if (s.empty()) return;
    size_t i = s.size() - 1;
    while (i > 0 && (static_cast<unsigned char>(s[i]) & 0xC0) == 0x80) --i;
    s.erase(i);
}

std::string two(int v) { char b[8]; std::snprintf(b, sizeof b, "%02d", v); return b; }
std::string clock(int minute) { return two(minute / 60) + ":" + two(minute % 60); }

const char* kDayNames[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

std::string statusText(const gf::Fighter& f) {
    std::string s;
    auto add = [&](const char* n, int t = -1) {
        s += n;
        if (t > 0) s += std::to_string(t);
        s += "  ";
    };
    if (f.sleepy) add("Sleepy");
    if (f.charging) add("Charging");
    if (f.wound) add("Wound", f.wound);
    if (f.daze) add("Daze", f.daze);
    if (f.slow) add("Slow", f.slow);
    if (f.haste) add("Haste", f.haste);
    if (f.armor) add("Armor", f.armor);
    if (f.thorns) add("Thorns", f.thorns);
    if (f.regen) add("Regen", f.regen);
    if (f.shield) add("Shield");
    if (f.focus) add("Focus");
    if (f.mark) add("Mark");
    if (f.trap) add("Trap", f.trap);
    if (f.stun) add("Stun");
    if (f.doom) add("Doom", f.doom);
    if (f.seal) add("Seal");
    if (f.evade) add("Evade");
    if (f.counterMult || f.counterHitPower || f.counterAbsorb) add("Counter");
    if (f.reflectDamage || f.reflectDebuff) add("Reflect");
    if (f.empowerN) add("Empowered");
    if (f.plates) add("Plates", f.plates);
    if (f.rebirth) add("Rebirth");
    return s;
}

const char* roleText(int speciesId) {
    switch (speciesId) {
        case 0: return "Stalwart: a sturdy tank that regrows and bristles with thorns.";
        case 3: return "Swift: fast and tricky, wins by acting first and dodging.";
        case 6: return "Ember: glass cannon that sets up huge burst hits.";
        default: return "";
    }
}

}  // namespace

// ---------------------------------------------------------------------------

Ui::Ui(gf::GameState& g) : g_(g) {}

void Ui::toast(const std::string& s) { toasts_.push_back({s, 6.f}); if (toasts_.size() > 6) toasts_.erase(toasts_.begin()); }

std::vector<std::string> Ui::takeBalloons() {
    std::vector<std::string> out;
    out.swap(balloons_);
    return out;
}

float Ui::desiredHeight() const { return tab_ == Tab::None ? kStrip : kStrip + kPanelH + 8; }

bool Ui::hover(float x, float y, float w, float h) const {
    return in_->mx >= x && in_->my >= y && in_->mx < x + w && in_->my < y + h;
}

bool Ui::button(float x, float y, float w, float h, const std::string& label, gfx::Color col, bool enabled, float fs, bool modal) {
    const bool allowed = enabled && (modal || !blocked_);
    const bool hv = allowed && hover(x, y, w, h);
    gfx::Color c = col;
    if (!enabled) c = rgba(48, 52, 64);
    else if (hv) { c.r = std::min(1.f, c.r + 0.10f); c.g = std::min(1.f, c.g + 0.10f); c.b = std::min(1.f, c.b + 0.10f); }
    gx_->fillRect(x, y, w, h, c, 6);
    gx_->text(label, x + 4, y, w - 8, h, fs, enabled ? kText : kDim, TF_CENTER | TF_VCENTER | TF_BOLD);
    if (hv && in_->pressed && !consumed_) { consumed_ = true; return true; }
    return false;
}

bool Ui::textField(int id, float x, float y, float w, float h, std::string& s, const std::string& placeholder) {
    bool focused = focusField_ == id;
    if (!blocked_ && hover(x, y, w, h) && in_->pressed && !consumed_) {
        focusField_ = id;
        focused = true;
        consumed_ = true;
        fieldClicked_ = true;
    }
    gx_->fillRect(x, y, w, h, rgba(14, 17, 26), 6);
    gx_->strokeRect(x, y, w, h, focused ? kAccent : rgba(70, 80, 110), 1.5f, 6);
    bool submitted = false;
    if (focused) {
        for (wchar_t c : in_->typed)
            if (c >= 32 && s.size() < 80) s += narrow(std::wstring(1, c));
        for (wchar_t c : in_->paste)
            if (c >= 32 && s.size() < 80) s += narrow(std::wstring(1, c));
        if (in_->backspace) popUtf8(s);
        if (in_->enter) submitted = true;
    }
    if (s.empty() && !focused) gx_->text(placeholder, x + 10, y, w - 20, h, 13, kDim, TF_VCENTER);
    else {
        std::string shown = s;
        if (focused && (nowMs_ / 500) % 2 == 0) shown += "|";
        gx_->text(shown, x + 10, y, w - 20, h, 13, kText, TF_VCENTER);
    }
    return submitted;
}

bool Ui::checkbox(float x, float y, const std::string& label, bool value, bool& changed) {
    const bool hv = !blocked_ && hover(x, y, 320, 24);
    gx_->fillRect(x, y + 3, 18, 18, rgba(14, 17, 26), 4);
    gx_->strokeRect(x, y + 3, 18, 18, hv ? kAccent : rgba(90, 100, 130), 1.5f, 4);
    if (value) gx_->fillRect(x + 4, y + 7, 10, 10, kGreen, 2);
    gx_->text(label, x + 26, y, 300, 24, 12, kText, TF_VCENTER);
    changed = false;
    if (hv && in_->pressed && !consumed_) { consumed_ = true; changed = true; return !value; }
    return value;
}

void Ui::card(float x, float y, float w, float h, gfx::Color c) { gx_->fillRect(x, y, w, h, c, 8); }

void Ui::bar(float x, float y, float w, float h, double frac, gfx::Color c) {
    gx_->fillRect(x, y, w, h, rgba(10, 12, 18), h / 2);
    frac = std::clamp(frac, 0.0, 1.0);
    if (frac > 0) gx_->fillRect(x, y, std::max(h, static_cast<float>(w * frac)), h, c, h / 2);
}

// ---------------------------------------------------------------------------
// Frame
// ---------------------------------------------------------------------------

void Ui::backgroundTick(long long nowMs, tu::Time now) {
    nowMs_ = nowMs;
    now_ = now;
    // Focus timer completion: the Battle button lights up and a notification is sent (no popup).
    if (focusRunning_ && nowMs_ >= focusEndMs_) {
        focusRunning_ = false;
        const gf::Result r = gf::completeFocus(g_, focusMinutes_, now_);
        for (const auto& m : r.messages) toast(m);
        balloon("Focus session complete! Your friends are ready for a battle whenever you are.");
        focusDone_ = focusMinutes_;
        battleReady_ = true;
        dirty_ = true;
    }
}

void Ui::refreshDue() {
    dueTimer_ = 1.0f;
    dueTab_ = dueNow_ = 0;
    for (const auto& o : gf::occurrencesOn(g_, now_.day, now_.minute)) {
        if (o.done || o.missed) continue;
        ++dueTab_;
        if (now_.minute >= o.win.start) ++dueNow_;
    }
}

void Ui::frame(gfx::Gfx& gx, float W, float H, Input& in, double dt, long long nowMs, tu::Time now) {
    gx_ = &gx; in_ = &in; W_ = W; H_ = H; nowMs_ = nowMs; now_ = now;
    consumed_ = false; blocked_ = false; fieldClicked_ = false;
    animT_ += static_cast<float>(dt);
    dt_ = static_cast<float>(dt);
    if (nagSeconds_ > 0) nagSeconds_ -= static_cast<float>(dt);

    if (!g_.started && tab_ == Tab::None) tab_ = Tab::Start;

    animating_ = false;
    backgroundTick(nowMs, now);
    dueTimer_ -= static_cast<float>(dt);
    if (dueTimer_ <= 0 || dirty_) refreshDue();
    // During a focus session only the Focus tab is reachable.
    if (focusRunning_ && tab_ != Tab::None && tab_ != Tab::Focus) tab_ = Tab::Focus;
    if (!toasts_.empty()) animating_ = true;
    for (auto& t : toasts_) t.life -= static_cast<float>(dt);
    toasts_.erase(std::remove_if(toasts_.begin(), toasts_.end(), [](const Toast& t) { return t.life <= 0; }), toasts_.end());

    if (in.escape) {
        if (focusField_ >= 0) focusField_ = -1;
        else if (tab_ != Tab::None && tab_ != Tab::Start) tab_ = Tab::None;
    }

    drawWorld(static_cast<float>(dt));
    if (tab_ != Tab::None) drawPanel();

    // Toasts: stacked near the top of the strip, or above the panel.
    float ty = tab_ == Tab::None ? H_ - kStrip + 6 : H_ - kStrip - 4 - 28.f * static_cast<float>(toasts_.size());
    if (tab_ != Tab::None) ty = std::max(4.f, ty);
    for (const auto& t : toasts_) {
        const float w = std::min(W_ - 20, gx_->textWidth(t.text, 13, true) + 28);
        const float a = std::min(1.f, t.life / 1.f);
        gx_->fillRect((W_ - w) / 2, ty, w, 24, gfx::withAlpha(rgba(20, 24, 36), 0.92f * a), 12);
        gx_->text(t.text, (W_ - w) / 2, ty, w, 24, 13, gfx::withAlpha(kText, a), TF_CENTER | TF_VCENTER | TF_BOLD);
        ty += 28;
    }
    if (in.pressed && !fieldClicked_) focusField_ = -1;
}

// ---------------------------------------------------------------------------
// World strip
// ---------------------------------------------------------------------------

namespace {
// Grass palette keyframes by hour; colours blend smoothly between them.
struct GrassPal { gfx::Color g1, g2, g3, g4; };
struct GrassKey { float hour; GrassPal pal; };
gfx::Color rgbf(int r, int g, int b) { return gfx::Color{r / 255.f, g / 255.f, b / 255.f, 1.f}; }
const GrassPal kNight{rgbf(76, 142, 140), rgbf(52, 104, 112), rgbf(38, 76, 92), rgbf(30, 58, 78)};
const GrassPal kDawn{rgbf(190, 214, 128), rgbf(150, 186, 112), rgbf(112, 150, 98), rgbf(92, 126, 88)};
const GrassPal kDay{rgbf(132, 210, 104), rgbf(88, 172, 84), rgbf(62, 136, 72), rgbf(50, 112, 62)};
const GrassPal kNoon{rgbf(146, 220, 112), rgbf(98, 184, 88), rgbf(68, 146, 76), rgbf(54, 120, 64)};
const GrassPal kDusk{rgbf(214, 190, 104), rgbf(170, 156, 86), rgbf(126, 118, 76), rgbf(98, 92, 64)};

gfx::Color mixc(gfx::Color a, gfx::Color b, float t) { return gfx::Color{a.r + (b.r - a.r) * t, a.g + (b.g - a.g) * t, a.b + (b.b - a.b) * t, 1.f}; }

GrassPal grassAt(double hour) {
    static const GrassKey keys[] = {{0.f, kNight}, {5.5f, kNight}, {6.5f, kDawn}, {9.f, kDay}, {13.f, kNoon}, {17.f, kDay}, {18.5f, kDusk}, {20.5f, kNight}, {24.f, kNight}};
    const int n = static_cast<int>(sizeof(keys) / sizeof(keys[0]));
    for (int i = 0; i + 1 < n; ++i) {
        if (hour >= keys[i].hour && hour <= keys[i + 1].hour) {
            const float t = (static_cast<float>(hour) - keys[i].hour) / (keys[i + 1].hour - keys[i].hour);
            return {mixc(keys[i].pal.g1, keys[i + 1].pal.g1, t), mixc(keys[i].pal.g2, keys[i + 1].pal.g2, t),
                    mixc(keys[i].pal.g3, keys[i + 1].pal.g3, t), mixc(keys[i].pal.g4, keys[i + 1].pal.g4, t)};
        }
    }
    return kNight;
}

// One ground cell is one art pixel: the same size as a sprite pixel (2 logical px at 100% scale), snapped to whole
// device pixels so nothing blurs at 125% or 150% display scaling.
float groundCell(const gfx::Gfx& g) {
    const float scale = g.scale();
    return std::max(1.f, std::round(2.f * scale)) / scale;
}
}  // namespace

void Ui::drawGround(float gy, float gh) {
    // The ground is thousands of tiny rectangles, so it is drawn once into a bitmap and re-drawn only when its colours
    // change (once a minute), the window size changes or the display scale changes.
    constexpr int kGroundCache = 1;
    const float cell = groundCell(*gx_);
    const float top = 6.f * cell;  // room above the grass line for blades and flowers
    const long long key = (static_cast<long long>(W_ * 4) << 24) ^ (static_cast<long long>(cell * 64) << 12) ^ static_cast<long long>(now_.minute) ^ (static_cast<long long>(gh) << 44);
    if (!gx_->hasCache(kGroundCache, key) && gx_->beginCache(kGroundCache, key, W_, gh + top)) {
        paintGround(top, gh);
        gx_->endCache();
    }
    if (!gx_->drawCache(kGroundCache, 0, gy - top)) paintGround(gy, gh);  // fallback if caching is unavailable
}

void Ui::paintGround(float gy, float gh) {
    const float cell = groundCell(*gx_);
    const double hour = now_.minute / 60.0;
    const GrassPal P = grassAt(hour);
    const int cols = static_cast<int>(std::ceil(W_ / cell)) + 1;
    const int rows = std::max(4, static_cast<int>(gh / cell));
    static const int kBlade[8] = {2, 3, 2, 1, 3, 2, 1, 2};
    auto cellRect = [&](int c, int r, int w, int h, gfx::Color col, float a) { gx_->fillRect(static_cast<float>(c) * cell, gy + static_cast<float>(r) * cell, static_cast<float>(w) * cell, static_cast<float>(h) * cell, gfx::withAlpha(col, a)); };

    // Body: three flat layers, each a little darker and more see-through, with a checker dither where they meet.
    auto bandOf = [&](int r) { return r <= 2 ? 0 : r <= 10 ? 1 : 2; };
    const gfx::Color bandCol[3] = {P.g2, P.g3, P.g4};
    const float bandAlpha[3] = {0.80f, 0.66f, 0.52f};
    int r = 1;
    while (r <= rows) {
        const int b = bandOf(r);
        int r2 = r;
        while (r2 + 1 <= rows && bandOf(r2 + 1) == b) ++r2;
        cellRect(0, r, cols, r2 - r + 1, bandCol[b], bandAlpha[b]);
        r = r2 + 1;
    }
    // the highlight row under the blades, then dithered seams between layers
    cellRect(0, 0, cols, 1, P.g2, 0.80f);
    for (int c = 0; c < cols; c += 2) {
        cellRect(c, 3, 1, 1, P.g3, 0.80f);          // seam into layer 2
        cellRect(c + 1, 11, 1, 1, P.g4, 0.66f);     // seam into layer 3
    }
    // Texture inside the grass. Pick one with kTuftStyle: 0 plain layers, 1 staggered tufts, 2 even specks, 3 short
    // dashes, 4 highlight rows. All of them are laid out in offset rows (like bricks), never on a diagonal.
    constexpr int kTuftStyle = 1;
    auto lighter = [&](int row) { return mixc(bandCol[bandOf(row)], P.g1, 0.22f); };
    if (kTuftStyle == 1) {
        for (int ri = 0, row = 3; row + 1 <= rows; row += 4, ++ri)
            for (int c = (ri % 2) * 8 + 3; c < cols; c += 16) {
                cellRect(c, row, 1, 1, lighter(row), 0.92f);
                cellRect(c - 1, row + 1, 1, 1, lighter(row), 0.8f);
                cellRect(c + 1, row + 1, 1, 1, lighter(row), 0.8f);
            }
    } else if (kTuftStyle == 2) {
        for (int ri = 0, row = 3; row <= rows; row += 3, ++ri)
            for (int c = (ri % 2) * 6 + 2; c < cols; c += 12) cellRect(c, row, 1, 1, lighter(row), 0.92f);
    } else if (kTuftStyle == 3) {
        for (int ri = 0, row = 4; row <= rows; row += 4, ++ri)
            for (int c = (ri % 2) * 8 + 2; c < cols; c += 16) cellRect(c, row, 2, 1, lighter(row), 0.88f);
    } else if (kTuftStyle == 4) {
        for (int row : {1, 4, 12})
            if (row <= rows) cellRect(0, row, cols, 1, mixc(bandCol[bandOf(row)], P.g1, 0.16f), 0.85f);
    }
    // Blades along the top edge: a repeating 8-column pattern, lightest tip.
    for (int c = 0; c < cols; ++c) {
        const int h = kBlade[c % 8];
        for (int k = 0; k < h; ++k) cellRect(c, -k, 1, 1, k == h - 1 ? P.g1 : P.g2, k == h - 1 ? 0.92f : 0.80f);
    }
    // Sparse flowers by day (every 24 cells), in four colours.
    if (hour >= 6.0 && hour < 20.0) {
        const gfx::Color petals[4] = {rgbf(255, 110, 150), rgbf(255, 226, 90), rgbf(255, 255, 255), rgbf(150, 190, 255)};
        for (int c = 10, i = 0; c < cols - 1; c += 24, ++i) {
            for (int k = 1; k <= 3; ++k) cellRect(c, -k, 1, 1, P.g3, 0.95f);
            const gfx::Color pc = petals[i % 4];
            cellRect(c - 1, -4, 1, 1, pc, 1.f); cellRect(c + 1, -4, 1, 1, pc, 1.f); cellRect(c, -5, 1, 1, pc, 1.f);
            cellRect(c, -4, 1, 1, i % 3 ? rgbf(255, 214, 90) : rgbf(255, 255, 255), 1.f);
        }
    }
}

void Ui::drawWorld(float dt) {
    const float gh = 54.f, gy = H_ - gh;
    drawGround(gy, gh);
    const bool lock = locked();

    // Walkers follow the party.
    std::vector<int> ids = g_.party;
    walkers_.erase(std::remove_if(walkers_.begin(), walkers_.end(), [&](const Walker& w) { return std::find(ids.begin(), ids.end(), w.friendId) == ids.end(); }), walkers_.end());
    for (int id : ids) {
        if (std::none_of(walkers_.begin(), walkers_.end(), [&](const Walker& w) { return w.friendId == id; })) {
            Walker w;
            w.friendId = id;
            w.rng = static_cast<unsigned>(id) * 2654435761u + 12345u;
            walkers_.push_back(w);
        }
    }
    const bool sleepy = gf::isSleepy(g_, now_.day);
    const int lead = gf::leadFriendId(g_);
    const float cell = groundCell(*gx_);
    const int minC = 14, maxC = std::max(minC + 4, static_cast<int>(W_ / cell) - 14);
    auto next = [](Walker& w) { w.rng = w.rng * 1664525u + 1013904223u; return (w.rng >> 8) & 0xFFFF; };
    groupT_ -= dt;
    if (groupT_ <= 0) {
        groupRng_ = groupRng_ * 1664525u + 1013904223u;
        groupWalk_ = !groupWalk_;
        groupT_ = groupWalk_ ? 4.f + static_cast<float>((groupRng_ >> 8) % 40) / 10.f : 3.f + static_cast<float>((groupRng_ >> 8) % 30) / 10.f;
    }
    for (auto& w : walkers_) {
        const gf::Friend* f = gf::findFriend(g_, w.friendId);
        if (!f) continue;
        const float size = 24.f * cell;  // every friend uses the same sprite-pixel size as the ground
        if (!w.init) {
            w.init = true;
            w.cx = minC + static_cast<int>(next(w) % static_cast<unsigned>(std::max(1, maxC - minC)));
            w.dir = (next(w) & 1) ? 1 : -1;
            static const float kSpeeds[5] = {5.f, 7.5f, 10.f, 12.5f, 15.f};   // cells per second: steady, differing rhythms
            w.cps = kSpeeds[next(w) % 5];
            w.walkT = 1.5f + static_cast<float>(next(w) % 30) / 10.f;
            w.animRate = 3.4f + static_cast<float>(next(w) % 25) / 10.f;       // 3.4 to 5.9 walk frames per second
            w.animT = static_cast<float>(next(w) % 400) / 100.f;               // random starting phase
            w.idleRate = 1.1f + static_cast<float>(next(w) % 14) / 10.f;
            w.idleT = static_cast<float>(next(w) % 300) / 100.f;
        }
        bool stepped = false;
        if (!sleepy && groupWalk_) {
            if (w.pause > 0) {
                w.pause -= dt;
                if (w.pause <= 0) {                       // rest is over: maybe turn around, then walk for a while
                    if (next(w) % 3 == 0) w.dir = -w.dir;
                    w.walkT = 2.5f + static_cast<float>(next(w) % 25) / 10.f;
                }
            } else {
                animating_ = true;
                w.stepAcc += w.cps * dt;                  // each friend advances at its own pace, one whole cell at a time
                while (w.stepAcc >= 1.f) {
                    w.stepAcc -= 1.f;
                    w.cx += w.dir;
                    stepped = true;
                    if (w.cx < minC) { w.cx = minC; w.dir = 1; }
                    if (w.cx > maxC) { w.cx = maxC; w.dir = -1; }
                }
                w.walkT -= dt;
                if (w.walkT <= 0) w.pause = 1.5f + static_cast<float>(next(w) % 30) / 10.f;
            }
        }
        if (stepped) w.moveT = 0.35f; else w.moveT = std::max(0.f, w.moveT - dt);
        if (w.hop > 0 || w.hopV != 0) {
            animating_ = true;
            w.hopV -= 600 * dt;
            w.hop += w.hopV * dt;
            if (w.hop <= 0) { w.hop = 0; w.hopV = 0; }
        }
        // Animation: a four-frame waddle while moving (lean left, stand, lean right, stand, with a hop on the stand frames),
        // a slow breathing and blinking idle otherwise. Every friend runs on its own clock and rate.
        int frame;
        float bob = 0.f;
        if (sleepy) {
            frame = 1;
        } else if (w.moveT > 0.f) {
            w.animT += dt * w.animRate;
            static const int kWalk[4] = {2, 0, 3, 1};
            const int idx = static_cast<int>(w.animT) % 4;
            frame = kWalk[idx];
            bob = (idx % 2) ? cell : 0.f;
        } else {
            w.idleT += dt * w.idleRate;
            frame = static_cast<int>(w.idleT) % 5 == 4 ? 1 : 0;   // mostly eyes open, a quick blink and flutter now and then
            if (static_cast<int>(w.idleT * 0.5f) % 2) bob = 0.f;
        }
        const float hopSnap = std::round(w.hop / cell) * cell;   // hops land on the grid too
        const float xPx = static_cast<float>(w.cx) * cell;
        const float sx = xPx - size / 2, sy = gy - 20.f * cell - bob - hopSnap;
        // small oval shadow on the ground grid
        gx_->fillRect(sx + 5.f * cell, gy + 2.f * cell, 14.f * cell, cell, rgba(0, 0, 0, 52));
        gx_->fillRect(sx + 7.f * cell, gy + 3.f * cell, 10.f * cell, cell, rgba(0, 0, 0, 38));
        gx_->sprite(gfx::SpriteKind::Species, f->species, frame, w.dir < 0, sx, sy, size);
        if (sleepy) gx_->text("z z", xPx - 10, sy - 6, 40, 16, 12, kText, TF_BOLD);
        if (!lock && !blocked_ && !consumed_ && in_->pressed && hover(sx, sy, size, size) && !(panel_.w > 0 && tab_ != Tab::None && hover(panel_.x, panel_.y, panel_.w, panel_.h))) {
            consumed_ = true;
            w.hopV = 190;
            w.hop = 0.01f;
            selFriend_ = w.friendId;
            tab_ = Tab::Friends;
        }
        // Nag bubble on the lead friend.
        if (w.friendId == lead && tab_ == Tab::None) {
            const int due = dueNow_;
            std::string t;
            if (focusRunning_) t = "Stay focused!";
            else if (battleReady_) t = "Battle time?";
            else if (due > 0) t = std::to_string(due) + " to do!";
            else if (nagSeconds_ > 0) t = "Hey!";
            if (!t.empty()) {
                animating_ = true;  // the bubble bobs
                const float bw = gx_->textWidth(t, 12, true) + 18;
                const float by = sy - 26 + (std::sin(animT_ * 4) * 2);
                gx_->fillRect(xPx - bw / 2, by, bw, 22, rgba(255, 255, 255, 235), 11);
                gx_->text(t, xPx - bw / 2, by, bw, 22, 12, rgba(30, 30, 40), TF_CENTER | TF_VCENTER | TF_BOLD);
            }
        }
    }

    // HUD on a soft dark pill so it stays readable on the translucent ground.
    std::string hud = "Energy " + std::to_string(g_.energy) + "   Coins " + std::to_string(g_.coins) + "   Streak " + std::to_string(g_.globalStreak) + "d";
    if (!g_.eggs.empty()) hud += "   Eggs " + std::to_string(g_.eggs.size());
    const float hw = gx_->textWidth(hud, 13, true) + 22;
    gx_->fillRect(8, gy + 6, hw, 22, rgba(10, 14, 24, 150), 11);
    gx_->text(hud, 19, gy + 6, hw, 22, 13, rgba(255, 255, 255), TF_VCENTER | TF_BOLD);
    std::string second;
    gfx::Color secondCol = rgba(255, 255, 255, 235);
    if (focusRunning_) {
        const long long left = std::max(0LL, (focusEndMs_ - nowMs_) / 1000);
        second = "Focus " + two(static_cast<int>(left / 60)) + ":" + two(static_cast<int>(left % 60)) + "  (everything else is locked)";
        secondCol = kAccent;
    } else if (sleepy) {
        second = "Your friends are sleepy. Finish a task to wake them!";
    }
    if (!second.empty()) {
        const float sw = gx_->textWidth(second, 12, true) + 22;
        gx_->fillRect(8, gy + 31, sw, 20, rgba(10, 14, 24, 150), 10);
        gx_->text(second, 19, gy + 31, sw, 20, 12, secondCol, TF_VCENTER | TF_BOLD);
    }

    struct TabBtn { Tab t; const char* label; };
    const TabBtn tabs[] = {{Tab::Tasks, "Tasks"}, {Tab::Friends, "Friends"}, {Tab::Dex, "Dex"}, {Tab::Shop, "Shop"}, {Tab::Focus, "Focus"}, {Tab::Battle, "Battle"}, {Tab::Guide, "Guide"}};
    const float bw = 74, bgap = 5;
    float bx = W_ - (bw + bgap) * 7 - 6;
    for (const auto& tb : tabs) {
        std::string label = tb.label;
        if (tb.t == Tab::Tasks) {
            if (dueTab_ > 0) label += " (" + std::to_string(dueTab_) + ")";
        }
        const bool active = tab_ == tb.t;
        const bool enabled = g_.started && (!lock || tb.t == Tab::Focus);
        gfx::Color col = active ? kAccent : rgba(40, 48, 70, 225);
        if (tb.t == Tab::Battle && battleReady_ && !active) {
            label = "Battle!";
            const float pulse = 0.5f + 0.5f * std::sin(animT_ * 5.f);
            col = gfx::Color{0.55f + 0.4f * pulse, 0.35f + 0.25f * pulse, 0.12f, 1.f};
        }
        if (button(bx, gy + 12, bw, 30, label, col, enabled, 12) && enabled) {
            tab_ = active ? Tab::None : tb.t;
            if (tb.t == Tab::Battle) battleReady_ = false;
        }
        bx += bw + bgap;
    }
}

// ---------------------------------------------------------------------------
// Panel frame and tabs
// ---------------------------------------------------------------------------

void Ui::drawPanel() {
    const float pw = std::min(1000.f, W_ - 24.f);
    const float ph = H_ - kStrip - 8.f;
    panel_ = {(W_ - pw) / 2, 0, pw, ph};
    card(panel_.x, panel_.y, pw, ph, kBg);
    gx_->strokeRect(panel_.x, panel_.y, pw, ph, rgba(80, 92, 130), 1.5f, 10);

    if (tab_ != Tab::Start) {
        struct T { Tab t; const char* n; };
        const T tabs[] = {{Tab::Tasks, "Tasks"}, {Tab::Friends, "Friends"}, {Tab::Dex, "Dex"}, {Tab::Shop, "Shop"}, {Tab::Focus, "Focus"}, {Tab::Battle, "Battle"}, {Tab::Guide, "Guide"}};
        float x = panel_.x + 12;
        for (const auto& t : tabs) {
            const bool enabled = !locked() || t.t == Tab::Focus;
            if (button(x, 10, 88, 30, t.n, tab_ == t.t ? kBlue : kBtn, enabled, 13)) {
                tab_ = t.t;
                if (t.t == Tab::Battle) battleReady_ = false;
            }
            x += 92;
        }
        gx_->text("Energy " + std::to_string(g_.energy) + "   Coins " + std::to_string(g_.coins), x + 10, 10, 300, 30, 13, kAccent, TF_VCENTER | TF_BOLD);
        if (button(panel_.x + pw - 44, 10, 32, 30, "X", rgba(120, 50, 50), true, 13)) tab_ = Tab::None;
    }
    const Rect body{panel_.x + 16, 54, pw - 32, ph - 70};
    switch (tab_) {
        case Tab::Start: drawStart(body); break;
        case Tab::Tasks: drawTasks(body); break;
        case Tab::Friends: drawFriends(body); break;
        case Tab::Dex: drawDex(body); break;
        case Tab::Shop: drawShop(body); break;
        case Tab::Focus: drawFocus(body); break;
        case Tab::Battle: drawBattle(body); break;
        case Tab::Guide: drawGuide(body); break;
        default: break;
    }
}

void Ui::drawStart(const Rect& b) {
    gx_->text("Welcome to Goal Friends!", b.x, 8, b.w, 34, 24, kAccent, TF_CENTER | TF_BOLD);
    gx_->text("Pick your first friend. Finish real-life tasks to level them up, hatch eggs, and earn Energy for battles.",
              b.x + 40, 46, b.w - 80, 44, 14, kText, TF_CENTER | TF_WRAP);
    const float cw = 270, gap = 24, total = cw * 3 + gap * 2;
    float x = b.x + (b.w - total) / 2;
    for (int id : gf::starterSpecies()) {
        card(x, 110, cw, 300);
        gx_->sprite(gfx::SpriteKind::Species, id, static_cast<int>(animT_ * 2) % 2, false, x + (cw - 120) / 2, 130, 120);
        gx_->text(gf::species(id).name, x, 262, cw, 26, 18, kText, TF_CENTER | TF_BOLD);
        gx_->text(roleText(id), x + 14, 292, cw - 28, 56, 12, kDim, TF_CENTER | TF_WRAP);
        if (button(x + 50, 358, cw - 100, 38, "Choose " + gf::species(id).name, kGreen, true, 13)) {
            const gf::Result r = gf::chooseStarter(g_, id, now_);
            for (const auto& m : r.messages) toast(m);
            dirty_ = true;
            tab_ = Tab::Tasks;
        }
        x += cw + gap;
    }
}

// ---------------------------------------------------------------------------
// Tasks
// ---------------------------------------------------------------------------

std::string Ui::scheduleText(const gf::Task& t) const {
    using gf::RecType;
    const auto& r = t.rec;
    switch (r.type) {
        case RecType::Once: return "once";
        case RecType::Daily: return "daily";
        case RecType::Weekdays: {
            if (r.weekdayMask == 0b0111110) return "weekdays";
            if (r.weekdayMask == 0b1000001) return "weekends";
            std::string s;
            for (int d = 1; d <= 7; ++d) if ((r.weekdayMask >> (d % 7)) & 1) { if (!s.empty()) s += " "; s += kDayNames[d % 7]; }
            return s.empty() ? "no days" : s;
        }
        case RecType::EveryN: return "every " + std::to_string(r.n) + " days";
        case RecType::Weekly: return std::string("weekly (") + kDayNames[((r.weekday % 7) + 7) % 7] + ")";
        case RecType::Monthly: return r.monthday <= 0 ? "monthly (last day)" : "monthly (day " + std::to_string(r.monthday) + ")";
    }
    return "";
}

std::string Ui::timeText(const gf::Task& t) const {
    if (t.targetMinute >= 0) return "at " + clock(t.targetMinute);
    switch (t.window) {
        case gf::Window::Morning: return "morning";
        case gf::Window::Afternoon: return "afternoon";
        case gf::Window::Evening: return "evening";
        default: return "any time";
    }
}

void Ui::completeNow(int taskId) {
    const gf::Result r = gf::completeTask(g_, taskId, now_);
    for (const auto& m : r.messages) toast(m);
    if (r.ok) dirty_ = true;
}

void Ui::submitTaskForm() {
    std::string title = formTitle_;
    while (!title.empty() && title.back() == ' ') title.pop_back();
    if (title.empty()) { toast("Give the task a name first."); return; }
    gf::Task t;
    t.title = title;
    t.effort = static_cast<gf::Effort>(formEffort_);
    t.remind = formRemind_;
    switch (formRepeat_) {
        case 0: t.rec.type = gf::RecType::Once; t.dueDay = now_.day; break;
        case 1: t.rec.type = gf::RecType::Daily; break;
        case 2: t.rec.type = gf::RecType::Weekdays; t.rec.weekdayMask = formDays_; if (formDays_ == 0) { toast("Pick at least one day."); return; } break;
        case 3: t.rec.type = gf::RecType::EveryN; t.rec.n = formN_; break;
        case 4: t.rec.type = gf::RecType::Weekly; t.rec.weekday = tu::weekday(now_.day); break;
        default: t.rec.type = gf::RecType::Monthly; t.rec.monthday = tu::civilFromDays(now_.day).d; break;
    }
    switch (formWindow_) {
        case 1: t.window = gf::Window::Morning; break;
        case 2: t.window = gf::Window::Afternoon; break;
        case 3: t.window = gf::Window::Evening; break;
        case 4: t.targetMinute = formHour_ * 60 + formMin_; break;
        default: break;
    }
    gf::addTask(g_, t, now_);
    formTitle_.clear();
    focusField_ = -1;
    toast("Task added: " + title);
    dirty_ = true;
}

void Ui::drawTasks(const Rect& b) {
    const float lw = std::min(560.f, b.w * 0.58f);
    // Sub-tabs
    if (button(b.x, b.y, 96, 28, "Today", taskView_ == 0 ? kBlue : kBtn)) taskView_ = 0;
    if (button(b.x + 102, b.y, 110, 28, "All tasks", taskView_ == 1 ? kBlue : kBtn)) taskView_ = 1;
    const float ly = b.y + 36, lh = b.h - 36;

    gx_->pushClip(b.x, ly, lw, lh);
    float& scroll = taskView_ == 0 ? scrollToday_ : scrollAll_;
    if (hover(b.x, ly, lw, lh)) scroll -= in_->wheel * 40;
    scroll = std::clamp(scroll, 0.f, std::max(0.f, taskContent_[taskView_] - lh));   // clamp before drawing
    const float rowH = 48;
    float y = ly - scroll;
    float contentH = 0;

    if (taskView_ == 0) {
        auto occ = gf::occurrencesOn(g_, now_.day, now_.minute);
        std::stable_sort(occ.begin(), occ.end(), [&](const gf::Occurrence& a, const gf::Occurrence& c) {
            const int ra = a.done ? 2 : a.missed ? 1 : 0, rc = c.done ? 2 : c.missed ? 1 : 0;
            if (ra != rc) return ra < rc;
            return a.win.start < c.win.start;
        });
        if (occ.empty()) gx_->text("Nothing due today. Add a task on the right!", b.x, y + 10, lw, 24, 13, kDim);
        for (const auto& o : occ) {
            const gf::Task* t = gf::findTask(g_, o.taskId);
            if (!t) continue;
            const bool visible = y + rowH > ly && y < ly + lh;
            if (visible) {
                gfx::Color bg = o.done ? rgba(28, 44, 36) : o.missed ? rgba(56, 30, 34) : kCard;
                card(b.x, y, lw, rowH - 4, bg);
                gx_->text(t->title, b.x + 12, y + 4, lw - 200, 22, 14, o.done ? kDim : kText, TF_BOLD);
                std::string detail = std::string(gf::effortName(t->effort)) + " · " + scheduleText(*t) + " · " + timeText(*t);
                if (t->rec.type != gf::RecType::Once && t->streak > 0) detail += " · streak " + std::to_string(t->streak);
                if (o.overdue) detail += " · overdue";
                gx_->text(detail, b.x + 12, y + 24, lw - 200, 18, 11, kDim);
                const float rx = b.x + lw - 12;
                if (o.done) {
                    gx_->text(o.late ? "Done (late)" : "Done", rx - 100, y, 100, rowH - 4, 13, kGreen, TF_RIGHT | TF_VCENTER | TF_BOLD);
                } else if (o.missed) {
                    gx_->text("Missed", rx - 100, y, 100, rowH - 4, 13, rgba(230, 120, 120), TF_RIGHT | TF_VCENTER | TF_BOLD);
                } else if (confirmTask_ == t->id) {
                    gx_->text("Really done?", rx - 210, y, 90, rowH - 4, 12, kAccent, TF_RIGHT | TF_VCENTER);
                    if (button(rx - 112, y + 7, 52, 28, "Yes", kGreen)) { completeNow(t->id); confirmTask_ = -1; }
                    if (button(rx - 56, y + 7, 52, 28, "No", kBtn)) confirmTask_ = -1;
                } else {
                    if (button(rx - 70, y + 7, 70, 28, "Done", kGreen)) {
                        if (g_.settings.confirmDone) confirmTask_ = t->id; else completeNow(t->id);
                    }
                    if ((t->targetMinute >= 0 || t->window != gf::Window::Any) && t->remind && button(rx - 146, y + 7, 70, 28, "Snooze", kBtn, true, 12)) {
                        gf::snoozeReminder(g_, t->id, now_.day, now_, 60);
                        toast("Snoozed for an hour.");
                        dirty_ = true;
                    }
                }
            }
            y += rowH;
        }
        contentH = static_cast<float>(occ.size()) * rowH;
    } else {
        if (g_.tasks.empty()) gx_->text("No tasks yet.", b.x, y + 10, lw, 24, 13, kDim);
        int removeId = -1;
        for (auto& t : g_.tasks) {
            const bool visible = y + rowH > ly && y < ly + lh;
            if (visible) {
                card(b.x, y, lw, rowH - 4, t.paused ? rgba(34, 34, 40) : kCard);
                gx_->text(t.title, b.x + 12, y + 4, lw - 190, 22, 14, t.paused ? kDim : kText, TF_BOLD);
                std::string detail = std::string(gf::effortName(t.effort)) + " · " + scheduleText(t) + " · " + timeText(t) + (t.rec.type != gf::RecType::Once ? " · streak " + std::to_string(t.streak) : "");
                gx_->text(detail, b.x + 12, y + 24, lw - 190, 18, 11, kDim);
                const float rx = b.x + lw - 12;
                if (button(rx - 60, y + 7, 60, 28, "Delete", rgba(130, 50, 50), true, 12)) removeId = t.id;
                if (t.rec.type != gf::RecType::Once && button(rx - 130, y + 7, 64, 28, t.paused ? "Resume" : "Pause", kBtn, true, 12)) { t.paused = !t.paused; dirty_ = true; }
            }
            y += rowH;
        }
        contentH = static_cast<float>(g_.tasks.size()) * rowH;
        if (removeId >= 0) { gf::removeTask(g_, removeId); dirty_ = true; }
    }
    taskContent_[taskView_] = contentH;
    scroll = std::clamp(scroll, 0.f, std::max(0.f, contentH - lh));
    gx_->popClip();

    // New task form
    const float fx = b.x + lw + 24, fw = b.w - lw - 24;
    float fy = b.y;
    gx_->text("New task", fx, fy, fw, 22, 16, kAccent, TF_BOLD);
    fy += 26;
    if (textField(1, fx, fy, fw, 32, formTitle_, "What do you want to get done?")) submitTaskForm();
    fy += 42;
    gx_->text("Effort (XP 10 / 25 / 60 / 120)", fx, fy, fw, 18, 11, kDim);
    fy += 18;
    const char* eff[] = {"S", "M", "L", "XL"};
    for (int i = 0; i < 4; ++i)
        if (button(fx + i * ((fw - 12) / 4 + 4), fy, (fw - 12) / 4, 28, eff[i], formEffort_ == i ? kBlue : kBtn)) formEffort_ = i;
    fy += 38;
    gx_->text("Repeat", fx, fy, fw, 18, 11, kDim);
    fy += 18;
    const char* reps[] = {"Once (today)", "Every day", "On chosen weekdays", "Every N days", "Weekly (this weekday)", "Monthly (this date)"};
    if (button(fx, fy, fw, 28, std::string(reps[formRepeat_]) + "   >", kBtn, true, 12)) formRepeat_ = (formRepeat_ + 1) % 6;
    fy += 34;
    if (formRepeat_ == 2) {
        const char* dn[] = {"M", "T", "W", "T", "F", "S", "S"};
        const int bit[] = {1, 2, 3, 4, 5, 6, 0};
        for (int i = 0; i < 7; ++i) {
            const bool on = (formDays_ >> bit[i]) & 1;
            if (button(fx + i * (fw / 7), fy, fw / 7 - 4, 26, dn[i], on ? kGreen : kBtn, true, 12)) formDays_ ^= (1 << bit[i]);
        }
        fy += 32;
    } else if (formRepeat_ == 3) {
        if (button(fx, fy, 30, 26, "-", kBtn)) formN_ = std::max(2, formN_ - 1);
        gx_->text("every " + std::to_string(formN_) + " days", fx + 34, fy, 120, 26, 12, kText, TF_CENTER | TF_VCENTER);
        if (button(fx + 158, fy, 30, 26, "+", kBtn)) formN_ = std::min(60, formN_ + 1);
        fy += 32;
    }
    gx_->text("Time", fx, fy, fw, 18, 11, kDim);
    fy += 18;
    const char* wins[] = {"Any time", "Morning (6-12)", "Afternoon (12-17)", "Evening (17-22)", "At a specific time"};
    if (button(fx, fy, fw, 28, std::string(wins[formWindow_]) + "   >", kBtn, true, 12)) formWindow_ = (formWindow_ + 1) % 5;
    fy += 34;
    if (formWindow_ == 4) {
        if (button(fx, fy, 28, 26, "-", kBtn)) formHour_ = (formHour_ + 23) % 24;
        gx_->text(two(formHour_), fx + 30, fy, 34, 26, 14, kText, TF_CENTER | TF_VCENTER | TF_BOLD);
        if (button(fx + 66, fy, 28, 26, "+", kBtn)) formHour_ = (formHour_ + 1) % 24;
        gx_->text(":", fx + 96, fy, 10, 26, 14, kText, TF_CENTER | TF_VCENTER);
        if (button(fx + 110, fy, 28, 26, "-", kBtn)) formMin_ = (formMin_ + 55) % 60;
        gx_->text(two(formMin_), fx + 140, fy, 34, 26, 14, kText, TF_CENTER | TF_VCENTER | TF_BOLD);
        if (button(fx + 176, fy, 28, 26, "+", kBtn)) formMin_ = (formMin_ + 5) % 60;
        fy += 32;
    }
    bool ch = false;
    formRemind_ = checkbox(fx, fy, "Remind me (notifications)", formRemind_, ch);
    fy += 28;
    if (button(fx, fy, fw, 34, "Add task", kGreen, true, 14)) submitTaskForm();
    fy += 44;
    const bool before = g_.settings.confirmDone;
    g_.settings.confirmDone = checkbox(fx, fy, "Ask \"really done?\" before completing", g_.settings.confirmDone, ch);
    if (before != g_.settings.confirmDone) dirty_ = true;
}

// ---------------------------------------------------------------------------
// Friends
// ---------------------------------------------------------------------------

void Ui::drawFriends(const Rect& b) {
    const float lw = 620;
    const float cw = 196, ch = 100, gap = 8;
    const int cols = 3;
    gx_->pushClip(b.x, b.y, lw, b.h);
    if (hover(b.x, b.y, lw, b.h)) scrollFriends_ -= in_->wheel * 40;
    const int rows = (static_cast<int>(g_.friends.size()) + cols - 1) / cols;
    const float content = rows * (ch + gap);
    scrollFriends_ = std::clamp(scrollFriends_, 0.f, std::max(0.f, content - b.h));
    for (size_t i = 0; i < g_.friends.size(); ++i) {
        const gf::Friend& f = g_.friends[i];
        const float x = b.x + static_cast<float>(i % cols) * (cw + gap);
        const float y = b.y + static_cast<float>(i / cols) * (ch + gap) - scrollFriends_;
        if (y + ch < b.y || y > b.y + b.h) continue;
        const bool sel = f.id == selFriend_;
        const bool inParty = std::find(g_.party.begin(), g_.party.end(), f.id) != g_.party.end();
        card(x, y, cw, ch, sel ? kCard2 : kCard);
        if (sel) gx_->strokeRect(x, y, cw, ch, kAccent, 2, 8);
        const gf::Species& sp = gf::species(f.species);
        gx_->sprite(gfx::SpriteKind::Species, f.species, static_cast<int>(animT_ * 2 + static_cast<float>(i)) % 2, false, x + 6, y + 14, 72);
        gx_->text(sp.name, x + 82, y + 6, cw - 84, 20, 13, kText, TF_BOLD);
        gx_->text("Lv " + std::to_string(f.level), x + 82, y + 28, 80, 18, 12, kAccent, TF_BOLD);
        if (f.level < gf::kMaxLevel) bar(x + 82, y + 52, cw - 94, 8, static_cast<double>(f.xp) / gf::xpToNext(f.level), kBlue);
        gx_->text(inParty ? (g_.party[0] == f.id ? "Party (lead)" : "Party") : "Box", x + 82, y + 66, cw - 86, 18, 11, inParty ? kGreen : kDim);
        if (hover(x, y, cw, ch) && in_->pressed && !consumed_ && !blocked_ && in_->my >= b.y && in_->my < b.y + b.h) { consumed_ = true; selFriend_ = f.id; }
    }
    gx_->popClip();

    // Detail pane
    const float dx = b.x + lw + 16, dw = b.w - lw - 16;
    const gf::Friend* sf = selFriend_ >= 0 ? gf::findFriend(g_, selFriend_) : nullptr;
    if (!sf && !g_.friends.empty()) { selFriend_ = g_.friends[0].id; sf = &g_.friends[0]; }
    if (!sf) { gx_->text("No friends yet.", dx, b.y, dw, 24, 14, kDim); return; }
    const gf::Species& sp = gf::species(sf->species);
    const gf::Stats st = gf::statsFor(sp, sf->level);
    card(dx, b.y, dw, b.h - 70);
    gx_->sprite(gfx::SpriteKind::Species, sf->species, static_cast<int>(animT_ * 2) % 2, false, dx + 8, b.y + 8, 96);
    gx_->text(sp.name, dx + 112, b.y + 10, dw - 120, 26, 18, kText, TF_BOLD);
    gx_->text("Stage " + std::to_string(sp.stage) + (sp.rare ? " · Rare" : "") + " · Lv " + std::to_string(sf->level) +
              (sp.evolvesTo >= 0 ? "  ·  Evolves at Lv " + std::to_string(sp.evolveLevel) : ""), dx + 112, b.y + 38, dw - 120, 18, 12, kDim);
    gx_->text(sp.role, dx + 112, b.y + 56, dw - 120, 18, 12, rgba(130, 200, 255), TF_BOLD);
    gx_->text("HP " + std::to_string(st.hp) + "   ATK " + std::to_string(st.atk) + "   DEF " + std::to_string(st.def), dx + 112, b.y + 76, dw - 120, 18, 12, kText, TF_BOLD);
    gx_->text("SPA " + std::to_string(st.spa) + "   SPD " + std::to_string(st.spd) + "   SPE " + std::to_string(st.spe), dx + 112, b.y + 94, dw - 120, 18, 12, kText, TF_BOLD);
    gx_->text(sp.blurb, dx + 10, b.y + 110, dw - 20, 32, 11, kDim, TF_WRAP);
    gx_->text("Ability: " + std::string(gf::abilityInfo(sp.ability).name), dx + 10, b.y + 144, dw - 20, 16, 12, rgba(255, 200, 90), TF_BOLD);
    gx_->text(gf::abilityInfo(sp.ability).desc, dx + 10, b.y + 160, dw - 20, 28, 11, kDim, TF_WRAP);
    float my = b.y + 192;
    for (const auto& m : sp.moves) {
        gx_->text(m.name, dx + 10, my, dw - 20, 18, 12, kAccent, TF_BOLD);
        gx_->text(describeMove(m), dx + 10, my + 16, dw - 20, 30, 11, kDim, TF_WRAP);
        my += 48;
    }
    // Buttons
    const bool inParty = std::find(g_.party.begin(), g_.party.end(), sf->id) != g_.party.end();
    const float by = b.y + b.h - 62;
    if (button(dx, by, 130, 30, inParty ? "Move to box" : "Add to party", kBtn, true, 12)) {
        const gf::Result r = gf::setPartyMember(g_, sf->id, !inParty);
        for (const auto& m : r.messages) toast(m);
        dirty_ = true;
    }
    if (button(dx + 138, by, 100, 30, "Make lead", kBtn, inParty && g_.party[0] != sf->id, 12)) {
        g_.party.erase(std::find(g_.party.begin(), g_.party.end(), sf->id));
        g_.party.insert(g_.party.begin(), sf->id);
        dirty_ = true;
    }
    // Eggs
    float ex = dx;
    gx_->text("Eggs:", ex, by + 36, 50, 24, 12, kDim, TF_VCENTER);
    ex += 48;
    if (g_.eggs.empty()) gx_->text("none (buy one in the Shop)", ex, by + 36, 220, 24, 12, kDim, TF_VCENTER);
    for (const auto& e : g_.eggs) {
        gx_->sprite(gfx::SpriteKind::Egg, 0, static_cast<int>(animT_ * 2) % 2, false, ex, by + 30, 24);
        gx_->text(std::to_string(e.progress) + "/" + std::to_string(e.needed), ex + 26, by + 36, 40, 24, 12, kText, TF_VCENTER | TF_BOLD);
        ex += 74;
    }
}

// ---------------------------------------------------------------------------
// Dex
// ---------------------------------------------------------------------------

void Ui::drawDex(const Rect& b) {
    const float cw = 94, ch = 84, gap = 6;
    const int cols = 7;
    int owned = 0;
    for (int i = 0; i < gf::kSpeciesCount; ++i) if (g_.dexOwned.count(i)) ++owned;
    gx_->text("Discovered " + std::to_string(owned) + " / " + std::to_string(gf::kSpeciesCount) + " friends", b.x, b.y - 2, 400, 22, 14, kAccent, TF_BOLD);
    const float gy0 = b.y + 24, gridH = b.h - 24;
    const int rowsN = (gf::kSpeciesCount + cols - 1) / cols;
    const float content = static_cast<float>(rowsN) * (ch + gap);
    if (hover(b.x, gy0, cols * (cw + gap), gridH)) scrollDex_ -= in_->wheel * 48;
    scrollDex_ = std::clamp(scrollDex_, 0.f, std::max(0.f, content - gridH));
    gx_->pushClip(b.x, gy0, cols * (cw + gap), gridH);
    for (int i = 0; i < gf::kSpeciesCount; ++i) {
        const float x = b.x + static_cast<float>(i % cols) * (cw + gap), y = gy0 + static_cast<float>(i / cols) * (ch + gap) - scrollDex_;
        if (y + ch < gy0 || y > gy0 + gridH) continue;
        const bool own = g_.dexOwned.count(i) > 0, seen = own || g_.dexSeen.count(i) > 0;
        card(x, y, cw, ch, i == selDex_ ? kCard2 : kCard);
        if (i == selDex_) gx_->strokeRect(x, y, cw, ch, kAccent, 2, 8);
        gx_->sprite(seen ? gfx::SpriteKind::Species : gfx::SpriteKind::Silhouette, i, 0, false, x + (cw - 48) / 2, y + 3, 48);
        gx_->text(seen ? gf::species(i).name : "???", x, y + 52, cw, 18, 11, own ? kText : kDim, TF_CENTER | TF_BOLD);
        gx_->text(own ? "owned" : seen ? "seen" : "", x, y + 66, cw, 16, 10, own ? kGreen : kDim, TF_CENTER);
        if (hover(x, y, cw, ch) && in_->my >= gy0 && in_->my < gy0 + gridH && in_->pressed && !consumed_ && !blocked_) { consumed_ = true; selDex_ = i; }
    }
    gx_->popClip();
    const float dx = b.x + cols * (cw + gap) + 14, dw = b.w - cols * (cw + gap) - 14;
    if (selDex_ >= 0) {
        const bool seen = g_.dexOwned.count(selDex_) || g_.dexSeen.count(selDex_);
        card(dx, b.y, dw, b.h);
        if (!seen) {
            gx_->sprite(gfx::SpriteKind::Silhouette, selDex_, 0, false, dx + 10, b.y + 10, 96);
            gx_->text("???", dx + 112, b.y + 24, dw - 110, 26, 18, kDim, TF_BOLD);
            gx_->text("Find this friend by hatching eggs, battling, or catching wild ones.", dx + 12, b.y + 110, dw - 24, 60, 12, kDim, TF_WRAP);
        } else {
            const gf::Species& sp = gf::species(selDex_);
            gx_->sprite(gfx::SpriteKind::Species, selDex_, static_cast<int>(animT_ * 2) % 2, false, dx + 10, b.y + 10, 96);
            gx_->text(sp.name, dx + 112, b.y + 14, dw - 120, 26, 18, kText, TF_BOLD);
            gx_->text("Stage " + std::to_string(sp.stage) + (sp.rare ? " · Rare" : ""), dx + 112, b.y + 42, dw - 120, 18, 12, kDim);
            gx_->text(sp.role, dx + 112, b.y + 60, dw - 120, 18, 12, rgba(130, 200, 255), TF_BOLD);
            auto pips = [](int n) { return std::string(static_cast<size_t>(n), '|'); };
            gx_->text("HP " + pips(sp.hp) + "   ATK " + pips(sp.atk) + "   DEF " + pips(sp.def), dx + 112, b.y + 78, dw - 120, 16, 11, kText, TF_BOLD);
            gx_->text("SPA " + pips(sp.spa) + "   SPD " + pips(sp.spd) + "   SPE " + pips(sp.spe), dx + 112, b.y + 94, dw - 120, 16, 11, kText, TF_BOLD);
            gx_->text(sp.blurb, dx + 12, b.y + 110, dw - 24, 32, 11, kDim, TF_WRAP);
            gx_->text("Ability: " + std::string(gf::abilityInfo(sp.ability).name), dx + 12, b.y + 144, dw - 24, 16, 12, rgba(255, 200, 90), TF_BOLD);
            gx_->text(gf::abilityInfo(sp.ability).desc, dx + 12, b.y + 160, dw - 24, 28, 11, kDim, TF_WRAP);
            float my = b.y + 192;
            for (const auto& m : sp.moves) {
                gx_->text(m.name, dx + 12, my, dw - 24, 18, 12, kAccent, TF_BOLD);
                gx_->text(describeMove(m), dx + 12, my + 16, dw - 24, 30, 11, kDim, TF_WRAP);
                my += 46;
            }
        }
    } else {
        gx_->text("Click a friend to see their moves.", dx, b.y + 8, dw, 24, 13, kDim);
    }
}

// ---------------------------------------------------------------------------
// Shop
// ---------------------------------------------------------------------------

void Ui::drawShop(const Rect& b) {
    gx_->text("You have " + std::to_string(g_.coins) + " coins", b.x, b.y, b.w, 26, 18, kAccent, TF_CENTER | TF_BOLD);
    const float cw = 320, ch = 300, gap = 30;
    const float x0 = b.x + (b.w - (cw * 2 + gap)) / 2, y0 = b.y + 44;
    // Egg
    card(x0, y0, cw, ch);
    gx_->sprite(gfx::SpriteKind::Egg, 0, static_cast<int>(animT_ * 2) % 2, false, x0 + (cw - 96) / 2, y0 + 16, 96);
    gx_->text("Egg  -  " + std::to_string(gf::kEggCost) + " coins", x0, y0 + 124, cw, 26, 18, kText, TF_CENTER | TF_BOLD);
    gx_->text("Hatches into a new friend after 5 completed tasks. Rare friends are possible!", x0 + 20, y0 + 156, cw - 40, 54, 12, kDim, TF_CENTER | TF_WRAP);
    gx_->text("You have " + std::to_string(g_.eggs.size()) + " incubating", x0, y0 + 214, cw, 18, 12, kDim, TF_CENTER);
    if (button(x0 + 70, y0 + 244, cw - 140, 36, "Buy Egg", kGreen, g_.coins >= gf::kEggCost, 14)) {
        const gf::Result r = gf::buyEgg(g_);
        for (const auto& m : r.messages) toast(m);
        dirty_ = true;
    }
    // Charm
    const float cx = x0 + cw + gap;
    card(cx, y0, cw, ch);
    gx_->fillEllipse(cx + cw / 2, y0 + 70, 40, 40, rgba(120, 90, 200));
    gx_->fillEllipse(cx + cw / 2, y0 + 70, 26, 26, rgba(190, 160, 255));
    gx_->fillEllipse(cx + cw / 2 - 8, y0 + 60, 8, 8, rgba(255, 255, 255, 200));
    gx_->text("Catch Charm  -  " + std::to_string(gf::kCharmCost) + " coins", cx, y0 + 124, cw, 26, 18, kText, TF_CENTER | TF_BOLD);
    gx_->text("Use it in a wild battle to befriend a wild friend. Works best when it's hurt.", cx + 20, y0 + 156, cw - 40, 54, 12, kDim, TF_CENTER | TF_WRAP);
    gx_->text("You have " + std::to_string(g_.charms), cx, y0 + 214, cw, 18, 12, kDim, TF_CENTER);
    if (button(cx + 70, y0 + 244, cw - 140, 36, "Buy Charm", kGreen, g_.coins >= gf::kCharmCost, 14)) {
        const gf::Result r = gf::buyCharm(g_);
        for (const auto& m : r.messages) toast(m);
        dirty_ = true;
    }
}

// ---------------------------------------------------------------------------
// Focus
// ---------------------------------------------------------------------------

void Ui::drawFocus(const Rect& b) {
    const float cx = b.x + b.w / 2;
    gx_->text("Focus session", b.x, b.y, b.w, 30, 22, kAccent, TF_CENTER | TF_BOLD);
    gx_->text("Work without distractions. While the timer runs the rest of the app is locked. When it ends you earn Energy and XP, and the Battle button lights up.", b.x + 100, b.y + 36, b.w - 200, 40, 13, kDim, TF_CENTER | TF_WRAP);
    if (focusRunning_) {
        const long long left = std::max(0LL, (focusEndMs_ - nowMs_) / 1000);
        const double frac = 1.0 - static_cast<double>(left) / (focusMinutes_ * 60.0);
        gx_->text(two(static_cast<int>(left / 60)) + ":" + two(static_cast<int>(left % 60)), b.x, b.y + 110, b.w, 90, 72, kText, TF_CENTER | TF_BOLD | TF_VCENTER);
        bar(cx - 250, b.y + 220, 500, 14, frac, kGreen);
        gx_->text("Stay focused. Your friends are cheering you on!", b.x, b.y + 244, b.w, 24, 13, kDim, TF_CENTER);
        gx_->text("Tasks, friends, shop and battles are locked until the session ends (or you cancel).", b.x, b.y + 268, b.w, 24, 12, kAccent, TF_CENTER);
        if (button(cx - 70, b.y + 290, 140, 38, "Cancel session", rgba(130, 50, 50), true, 13)) {
            focusRunning_ = false;
            toast("Focus session cancelled (no reward).");
        }
        return;
    }
    gx_->text("Choose a length", b.x, b.y + 100, b.w, 22, 14, kText, TF_CENTER | TF_BOLD);
    const int mins[] = {1, 15, 25, 45};
    const char* names[] = {"1 min (test)", "15 min", "25 min", "45 min"};
    for (int i = 0; i < 4; ++i)
        if (button(cx - 250 + i * 128, b.y + 134, 120, 44, names[i], focusMinutes_ == mins[i] ? kBlue : kBtn, true, 14)) focusMinutes_ = mins[i];
    gx_->text("Reward: " + std::to_string(std::max(1, (focusMinutes_ + 9) / 10)) + " Energy and " + std::to_string(focusMinutes_) + " XP", b.x, b.y + 192, b.w, 22, 13, kAccent, TF_CENTER);
    if (button(cx - 90, b.y + 232, 180, 48, "Start focusing", kGreen, true, 16)) {
        focusRunning_ = true;
        focusEndMs_ = nowMs_ + static_cast<long long>(focusMinutes_) * 60 * 1000;
        focusDone_ = 0;
        toast("Focus started. Good luck!");
    }
}

// ---------------------------------------------------------------------------
// Battle
// ---------------------------------------------------------------------------

void Ui::beginBattle(gf::BattleMode m) {
    std::string why;
    std::unique_ptr<gf::Battle> bt;
    if (!gf::startBattle(g_, m, g_.lastArea, now_, bt, why)) { toast(why); return; }
    battle_ = std::move(bt);
    battle_->recordEvents = true;
    mode_ = m;
    phase_ = Phase::Choose;
    chosen_ = {{false, false, false}};
    curSlot_ = 0;
    pendingMove_ = -1;
    resultLines_.clear();
    focusDone_ = 0;
    battleReady_ = false;
    playing_ = false;
    fastPlay_ = false;
    floats_.clear();
    for (auto& row : hpAnim_) for (float& v : row) v = -1;
    for (int sd = 0; sd < 2; ++sd) for (int k = 0; k < 3; ++k) flash_[sd][k] = flashGood_[sd][k] = glow_[sd][k] = 0;
    dirty_ = true;
    battleAdvance();
}

void Ui::battleAdvance() {
    if (!battle_) return;
    for (int k = 0; k < gf::kActiveSlots; ++k) {
        if (chosen_[static_cast<size_t>(k)] || !battle_->slotFilled(0, k) || !battle_->fighter(0, k).alive()) continue;
        const gf::Fighter& f = battle_->fighter(0, k);
        gf::Action a;
        a.type = gf::Action::UseMove;
        if (f.charging) { a.move = f.chargeMove; a.target = f.chargeTarget; }
        else if (f.stun > 0) { a.move = 0; a.target = {1, 0}; }
        else { curSlot_ = k; phase_ = Phase::Choose; pendingMove_ = -1; return; }
        acts_[static_cast<size_t>(k)] = a;
        chosen_[static_cast<size_t>(k)] = true;
    }
    battleResolve();
}

void Ui::battleResolve() {
    prevSnap_ = battle_->snapshot();
    playStart_ = battle_->events.size();
    battle_->resolveTurn(acts_);
    chosen_ = {{false, false, false}};
    playEnd_ = battle_->events.size();
    if (playEnd_ > playStart_) {
        playing_ = true;
        viewRel_ = -1;
        playTimer_ = 99.f;  // show the first event immediately
        floats_.clear();
    } else {
        battleAfterTurn();
    }
}

void Ui::battleAfterTurn() {
    playing_ = false;
    viewRel_ = -1;
    if (battle_->over()) { battleFinish(false); return; }
    if (!battle_->emptyPlayerSlotsNeedingReplacement().empty()) { phase_ = Phase::Replace; return; }
    battleAdvance();
}

void Ui::battleFinish(bool ran) {
    const gf::Result r = gf::finishBattle(g_, *battle_, mode_);
    resultLines_.clear();
    if (ran) resultLines_.push_back("You got away safely.");
    else if (battle_->winner == 0) resultLines_.push_back("You won the battle!");
    for (const auto& m : r.messages) resultLines_.push_back(m);
    phase_ = Phase::Done;
    dirty_ = true;
}

Ui::Rect Ui::cardRect(int side, int slot) const {
    const float cardW = (battleBody_.w - 20) / 3, cardH = 100, logH = 74;
    const float x = battleBody_.x + static_cast<float>(slot) * (cardW + 10);
    const float y = side == 1 ? battleBody_.y : battleBody_.y + cardH + 6 + logH + 6;
    return {x, y, cardW, cardH};
}

const gf::Fighter* Ui::shownFighter(int side, int slot, int* teamIdx) const {
    if (!battle_) return nullptr;
    int idx = -1;
    if (playing_) {
        const gf::Snapshot& sn = viewRel_ < 0 ? prevSnap_ : battle_->events[playStart_ + static_cast<size_t>(viewRel_)].snap;
        idx = sn.active[static_cast<size_t>(side)][static_cast<size_t>(slot)];
        if (idx < 0) return nullptr;
        if (teamIdx) *teamIdx = idx;
        return &sn.team[static_cast<size_t>(side)][static_cast<size_t>(idx)];
    }
    if (!battle_->slotFilled(side, slot)) return nullptr;
    idx = battle_->sides[static_cast<size_t>(side)].active[static_cast<size_t>(slot)];
    if (teamIdx) *teamIdx = idx;
    return &battle_->sides[static_cast<size_t>(side)].team[static_cast<size_t>(idx)];
}

int Ui::shownBench(int side) const {
    if (!battle_) return 0;
    if (!playing_) return static_cast<int>(battle_->benchAlive(side).size());
    const gf::Snapshot& sn = viewRel_ < 0 ? prevSnap_ : battle_->events[playStart_ + static_cast<size_t>(viewRel_)].snap;
    int n = 0;
    for (size_t i = 0; i < sn.team[static_cast<size_t>(side)].size(); ++i) {
        if (!sn.team[static_cast<size_t>(side)][i].alive()) continue;
        bool active = false;
        for (int a : sn.active[static_cast<size_t>(side)]) if (a == static_cast<int>(i)) active = true;
        if (!active) ++n;
    }
    return n;
}

void Ui::spawnEffects(const gf::Snapshot& before, const gf::Snapshot& after, gf::Ref actor) {
    auto slotOf = [](const gf::Snapshot& sn, int side, int idx) {
        for (int k = 0; k < 3; ++k) if (sn.active[static_cast<size_t>(side)][static_cast<size_t>(k)] == idx) return k;
        return -1;
    };
    for (int sd = 0; sd < 2; ++sd) {
        const size_t n = std::min(before.team[static_cast<size_t>(sd)].size(), after.team[static_cast<size_t>(sd)].size());
        for (size_t i = 0; i < n; ++i) {
            const int d = after.team[static_cast<size_t>(sd)][i].hp - before.team[static_cast<size_t>(sd)][i].hp;
            if (d == 0) continue;
            int k = slotOf(after, sd, static_cast<int>(i));
            if (k < 0) k = slotOf(before, sd, static_cast<int>(i));
            if (k < 0) continue;
            const Rect r = cardRect(sd, k);
            floats_.push_back({r.x + r.w * 0.5f - 18.f + static_cast<float>((static_cast<int>(i) * 17) % 30 - 15), r.y + 26.f,
                               (d < 0 ? "-" : "+") + std::to_string(std::abs(d)), d < 0 ? rgba(255, 110, 110) : rgba(120, 235, 140), 1.3f});
            if (d < 0) flash_[sd][k] = 0.4f; else flashGood_[sd][k] = 0.4f;
        }
    }
    if (actor.side >= 0 && actor.slot >= 0 && actor.slot < 3) glow_[actor.side][actor.slot] = 1.0f;
}

void Ui::stepPlayback() {
    const gf::Snapshot& before = viewRel_ < 0 ? prevSnap_ : battle_->events[playStart_ + static_cast<size_t>(viewRel_)].snap;
    const gf::Snapshot beforeCopy = before;  // viewRel_ is about to change
    ++viewRel_;
    const gf::Event& ev = battle_->events[playStart_ + static_cast<size_t>(viewRel_)];
    spawnEffects(beforeCopy, ev.snap, ev.actor);
}

void Ui::fighterCard(float x, float y, float w, float h, int side, int slot, bool highlight, bool targetable, bool* clicked) {
    if (clicked) *clicked = false;
    int ti = -1;
    const gf::Fighter* fp = shownFighter(side, slot, &ti);
    if (!fp || ti < 0 || ti > 5) {
        card(x, y, w, h, rgba(26, 30, 42));
        gx_->text("-", x, y, w, h, 20, kDim, TF_CENTER | TF_VCENTER);
        return;
    }
    const gf::Fighter& f = *fp;
    const bool dead = !f.alive();
    float& anim = hpAnim_[side][ti];
    const float target = static_cast<float>(f.hpPct());
    if (anim < 0) anim = target;
    anim += (target - anim) * std::min(1.f, dt_ * 7.f);

    gx_->fillRect(x, y, w, h, side == 1 ? rgba(52, 34, 40) : rgba(30, 42, 60), 8);
    if (flash_[side][slot] > 0) gx_->fillRect(x, y, w, h, rgba(255, 70, 70, static_cast<int>(170 * flash_[side][slot] / 0.4f)), 8);
    if (flashGood_[side][slot] > 0) gx_->fillRect(x, y, w, h, rgba(80, 230, 130, static_cast<int>(150 * flashGood_[side][slot] / 0.4f)), 8);
    if (glow_[side][slot] > 0) gx_->strokeRect(x, y, w, h, rgba(255, 220, 90, static_cast<int>(255 * std::min(1.f, glow_[side][slot]))), 3.f, 8);
    if (highlight) gx_->strokeRect(x, y, w, h, kAccent, 2.5f, 8);
    if (targetable) {
        const bool hv = hover(x, y, w, h);
        gx_->strokeRect(x, y, w, h, hv ? rgba(255, 255, 255) : kGreen, hv ? 3.f : 2.f, 8);
        if (hv && in_->pressed && !consumed_) { consumed_ = true; if (clicked) *clicked = true; }
    }
    const float shake = flash_[side][slot] > 0 ? std::sin(animT_ * 70.f) * 3.f * flash_[side][slot] / 0.4f : 0.f;
    const float lunge = glow_[side][slot] > 0.5f ? (side == 1 ? 6.f : -6.f) * (glow_[side][slot] - 0.5f) * 2.f : 0.f;
    gx_->sprite(gfx::SpriteKind::Species, f.speciesId, static_cast<int>(animT_ * 2 + static_cast<float>(slot)) % 2, side == 1, x + 6 + shake, y + 14 + lunge, 72, dead ? 0.25f : 1.f);
    gx_->text(std::string(side == 1 ? "Foe " : "") + f.sp().name + "  Lv " + std::to_string(f.level), x + 84, y + 6, w - 90, 20, 13, kText, TF_BOLD);
    bar(x + 84, y + 30, w - 96, 10, anim, anim > 0.5 ? kGreen : anim > 0.2 ? rgba(220, 170, 40) : kRed);
    gx_->text(std::to_string(f.hp) + " / " + std::to_string(f.maxHp), x + 84, y + 42, w - 96, 16, 11, kDim);
    gx_->text(statusText(f), x + 84, y + 60, w - 92, 38, 10, rgba(255, 220, 140), TF_WRAP);
}

void Ui::drawBattle(const Rect& b) {
    battleBody_ = b;
    const gf::Area& ar = gf::area(std::clamp(g_.lastArea, 0, gf::kAreaCount - 1));
    auto tint = [](unsigned rgbv, int a) { return rgba(static_cast<int>((rgbv >> 16) & 255), static_cast<int>((rgbv >> 8) & 255), static_cast<int>(rgbv & 255), a); };

    // ---- No battle running: area and mode selection ----
    if (!battle_) {
        const float lw = 250;
        gx_->text("Choose an area", b.x, b.y - 4, lw, 22, 14, kAccent, TF_BOLD);
        for (int i = 0; i < gf::kAreaCount; ++i) {
            const gf::Area& a = gf::area(i);
            const bool unlocked = gf::areaUnlocked(g_, i);
            const float y = b.y + 22 + static_cast<float>(i) * 60;
            const bool sel = g_.lastArea == i;
            gx_->fillRect(b.x - 2, y - 2, lw + 4, 56, sel ? kAccent : rgba(0, 0, 0, 0), 9);
            std::string label = a.name + "\n" + (unlocked ? "Lv +" + std::to_string(a.levelBonus) + "  ·  coins x" + std::to_string(a.coinMult).substr(0, 3)
                                                          : "Locked: win " + std::to_string(a.unlockWins) + " battles");
            if (button(b.x, y, lw, 52, label, unlocked ? tint(a.colorBottom, 255) : rgba(48, 52, 64), unlocked, 12) && unlocked) {
                g_.lastArea = i;
                dirty_ = true;
            }
        }
        const float rx = b.x + lw + 20, rw = b.w - lw - 20;
        float y0 = b.y;
        if (focusDone_ > 0) {
            card(rx, y0, rw, 38, rgba(36, 60, 44));
            gx_->text("Focus session complete! Your friends are ready to battle.", rx, y0, rw, 38, 14, kGreen, TF_CENTER | TF_VCENTER | TF_BOLD);
            y0 += 46;
        }
        gx_->fillRect(rx, y0, rw, 96, tint(ar.colorTop, 70), 10);
        gx_->text(ar.name, rx + 14, y0 + 4, rw - 28, 26, 19, kText, TF_BOLD);
        gx_->text(ar.desc, rx + 14, y0 + 30, rw - 28, 20, 12, kDim);
        float sx = rx + 14;
        gx_->text("Lives here:", sx, y0 + 56, 80, 28, 11, kDim, TF_VCENTER);
        sx += 78;
        for (int id : gf::baseSpecies()) {
            const gf::Species& sp = gf::species(id);
            if (!(sp.habitats & ar.habitat) || sp.rare) continue;
            if (sx + 40 > rx + rw - 10) break;
            gx_->sprite(gfx::SpriteKind::Species, id, 0, false, sx, y0 + 50, 40);
            sx += 42;
        }
        const float cy = y0 + 106;
        struct Mode { gf::BattleMode m; const char* name; const char* desc; };
        const Mode modes[] = {
            {gf::BattleMode::Wild, "Wild encounter", "1 to 3 wild friends. A Catch Charm can befriend one."},
            {gf::BattleMode::Duel, "Trainer duel", "A full team of foes. Win coins and maybe an Egg."},
            {gf::BattleMode::Boss, "Boss battle", "A powerful team. Needs a 3-day streak. Guaranteed rare friend."}};
        const float gap = 12, cw = (rw - gap * 2) / 3;
        const float chh = std::min(228.f, b.y + b.h - cy - 30);
        for (int i = 0; i < 3; ++i) {
            const float x = rx + static_cast<float>(i) * (cw + gap);
            card(x, cy, cw, chh);
            gx_->text(modes[i].name, x, cy + 12, cw, 24, 16, kText, TF_CENTER | TF_BOLD);
            gx_->text(std::to_string(gf::energyCost(modes[i].m)) + " Energy", x, cy + 38, cw, 20, 14, kAccent, TF_CENTER | TF_BOLD);
            gx_->text(modes[i].desc, x + 14, cy + 68, cw - 28, 60, 12, kDim, TF_CENTER | TF_WRAP);
            const bool locked = modes[i].m == gf::BattleMode::Boss && !gf::bossUnlocked(g_);
            const bool afford = g_.energy >= gf::energyCost(modes[i].m);
            if (locked) gx_->text("Locked: streak " + std::to_string(g_.globalStreak) + " / 3 days", x, cy + chh - 74, cw, 18, 11, rgba(230, 140, 140), TF_CENTER);
            else if (!afford) gx_->text("Not enough Energy", x, cy + chh - 74, cw, 18, 11, rgba(230, 140, 140), TF_CENTER);
            if (button(x + 36, cy + chh - 50, cw - 72, 36, "Fight!", kRed, !locked && afford, 14)) beginBattle(modes[i].m);
        }
        gx_->text("Energy: " + std::to_string(g_.energy) + "   ·   Friends are fully healed after every battle.", rx, b.y + b.h - 24, rw, 20, 12, kDim, TF_CENTER);
        return;
    }

    // ---- Active battle ----
    if (playing_ || !floats_.empty()) animating_ = true;
    // advance the replay
    if (playing_) {
        playTimer_ += dt_ * (fastPlay_ ? 3.f : 1.f);
        float stepLen = 0.8f;
        if (viewRel_ >= 0) {
            const std::string& t = battle_->events[playStart_ + static_cast<size_t>(viewRel_)].text;
            stepLen = t.rfind("--", 0) == 0 ? 0.35f : t.find(" uses ") != std::string::npos ? 1.0f : 0.8f;
        }
        if (playTimer_ >= stepLen) {
            playTimer_ = 0;
            if (playStart_ + static_cast<size_t>(viewRel_ + 1) < playEnd_) stepPlayback();
            else battleAfterTurn();
        }
    }
    for (int sd = 0; sd < 2; ++sd)
        for (int k = 0; k < 3; ++k) {
            flash_[sd][k] = std::max(0.f, flash_[sd][k] - dt_);
            flashGood_[sd][k] = std::max(0.f, flashGood_[sd][k] - dt_);
            glow_[sd][k] = std::max(0.f, glow_[sd][k] - dt_ * 0.9f);
        }

    const float cardH = 100, logH = 74;
    const float logY = b.y + cardH + 6, pY = logY + logH + 6, aY = pY + cardH + 6;
    gx_->fillRect(b.x - 8, b.y - 6, b.w + 16, b.h + 8, tint(ar.colorTop, 55), 10);
    gx_->fillRect(b.x - 8, b.y + (b.h) * 0.55f, b.w + 16, b.h * 0.45f + 2, tint(ar.colorBottom, 60), 10);

    const bool modalActive = !playing_ && (phase_ == Phase::SwitchPick || phase_ == Phase::Replace || phase_ == Phase::Done);
    const bool prevBlocked = blocked_;
    if (modalActive || playing_) blocked_ = true;

    std::vector<gf::Ref> targets;
    if (!playing_ && phase_ == Phase::Target && battle_->slotFilled(0, curSlot_)) targets = battle_->validTargets(0, curSlot_, pendingMove_);
    auto isTarget = [&](int side, int slot) {
        if (playing_) return false;
        if (phase_ == Phase::CharmTarget) return side == 1 && battle_->slotFilled(1, slot) && battle_->fighter(1, slot).alive();
        for (const auto& t : targets) if (t.side == side && t.slot == slot) return true;
        return false;
    };

    for (int k = 0; k < 3; ++k) {
        bool clicked = false;
        const Rect r = cardRect(1, k);
        fighterCard(r.x, r.y, r.w, r.h, 1, k, false, isTarget(1, k), &clicked);
        if (clicked) {
            if (phase_ == Phase::CharmTarget) {
                const bool ok = gf::useCharm(g_, *battle_, k);
                toast(ok ? "Charm worked!" : "The charm fizzled.");
                dirty_ = true;
                if (battle_->over()) battleFinish(false); else phase_ = Phase::Choose;
            } else {
                acts_[static_cast<size_t>(curSlot_)] = {gf::Action::UseMove, pendingMove_, {1, k}, -1};
                chosen_[static_cast<size_t>(curSlot_)] = true;
                battleAdvance();
            }
        }
    }
    gx_->text("Foes bench: " + std::to_string(shownBench(1)), b.x + b.w - 150, b.y + cardH - 18, 146, 16, 10, kDim, TF_RIGHT);

    // Log: shows the line being played, plus the lines before it.
    card(b.x, logY, b.w, logH, rgba(14, 17, 26, 235));
    std::vector<std::string> lines;
    if (playing_) {
        const size_t upto = playStart_ + static_cast<size_t>(std::max(viewRel_, -1) + 1);
        for (size_t i = playStart_; i < upto; ++i) lines.push_back(battle_->events[i].text);
        if (lines.empty() && playStart_ > 0) lines.push_back(battle_->events[playStart_ - 1].text);
    } else {
        lines = battle_->log;
    }
    const size_t first = lines.size() > 4 ? lines.size() - 4 : 0;
    for (size_t i = first; i < lines.size(); ++i) {
        const bool last = i + 1 == lines.size();
        const float ly = logY + 4 + static_cast<float>(i - first) * 16.f;
        gx_->text(lines[i], b.x + 14, ly, b.w - 28, 18, last ? 13.5f : 11.5f, last ? rgba(255, 255, 255) : kDim, last ? TF_BOLD : 0);
    }

    for (int k = 0; k < 3; ++k) {
        bool clicked = false;
        const Rect r = cardRect(0, k);
        const bool cur = !playing_ && phase_ != Phase::Done && k == curSlot_ && (phase_ == Phase::Choose || phase_ == Phase::Target || phase_ == Phase::SwitchPick);
        fighterCard(r.x, r.y, r.w, r.h, 0, k, cur, isTarget(0, k), &clicked);
        if (clicked) {
            acts_[static_cast<size_t>(curSlot_)] = {gf::Action::UseMove, pendingMove_, {0, k}, -1};
            chosen_[static_cast<size_t>(curSlot_)] = true;
            battleAdvance();
        }
    }
    gx_->text("Your bench: " + std::to_string(shownBench(0)) + "   Turn " + std::to_string(battle_->turn + (playing_ ? 0 : 1)) + "   " + ar.name, b.x + b.w - 330, pY + cardH - 18, 326, 16, 10, kDim, TF_RIGHT);

    // Floating damage / heal numbers
    for (auto& ft : floats_) {
        ft.life -= dt_;
        ft.y -= dt_ * 28.f;
        const float a = std::clamp(ft.life / 0.5f, 0.f, 1.f);
        gx_->text(ft.text, ft.x + 1.5f, ft.y + 1.5f, 80, 26, 20, rgba(0, 0, 0, static_cast<int>(200 * a)), TF_BOLD);
        gx_->text(ft.text, ft.x, ft.y, 80, 26, 20, gfx::withAlpha(ft.col, a), TF_BOLD);
    }
    floats_.erase(std::remove_if(floats_.begin(), floats_.end(), [](const FloatText& f) { return f.life <= 0; }), floats_.end());

    // Action area
    if (playing_) {
        blocked_ = prevBlocked;
        gx_->text("The turn is playing out...", b.x, aY + 2, 400, 22, 14, kAccent, TF_BOLD);
        gx_->text("Watch the log and the health bars. Floating numbers show damage (red) and healing (green).", b.x, aY + 26, 700, 20, 12, kDim);
        if (button(b.x, aY + 56, 140, 34, "Skip", kBtn)) {
            playing_ = false;
            floats_.clear();
            battleAfterTurn();
        }
        if (button(b.x + 148, aY + 56, 170, 34, fastPlay_ ? "Speed: fast" : "Speed: normal", fastPlay_ ? rgba(120, 90, 30) : kBtn)) fastPlay_ = !fastPlay_;
    } else if ((phase_ == Phase::Choose || phase_ == Phase::Target || phase_ == Phase::CharmTarget) && battle_->slotFilled(0, curSlot_)) {
        const gf::Fighter& f = battle_->fighter(0, curSlot_);
        const gf::Species& sp = f.sp();
        if (phase_ == Phase::Choose) {
            gx_->text("What should " + sp.name + " do?", b.x, aY, 360, 20, 14, kText, TF_BOLD);
            bool anyUsable = false;
            for (int m = 0; m < 4; ++m) if (battle_->moveUsable(0, curSlot_, m)) anyUsable = true;
            const float bw = 250, bh = 34;
            std::string hoverText;
            if (!anyUsable) {
                if (button(b.x, aY + 26, bw, bh, "Scuffle (basic attack)", kBtn)) {
                    pendingMove_ = 4;
                    const auto t = battle_->validTargets(0, curSlot_, 4);
                    if (t.size() == 1) { acts_[static_cast<size_t>(curSlot_)] = {gf::Action::UseMove, 4, t[0], -1}; chosen_[static_cast<size_t>(curSlot_)] = true; battleAdvance(); }
                    else phase_ = Phase::Target;
                }
            }
            for (int m = 0; m < 4; ++m) {
                const float x = b.x + static_cast<float>(m % 2) * (bw + 8), y = aY + 26 + static_cast<float>(m / 2) * (bh + 6);
                if (!anyUsable) break;
                const gf::Move& mv = sp.moves[static_cast<size_t>(m)];
                const bool ok = battle_->moveUsable(0, curSlot_, m);
                std::string label = mv.name;
                if (f.cd[m] > 0) label += "  (CD " + std::to_string(f.cd[m]) + ")";
                else if (mv.once && f.used[m]) label += "  (used)";
                else if (f.seal > 0 && f.sealMove == m) label += "  (sealed)";
                else if (!ok) label += "  (taunted)";
                if (hover(x, y, bw, bh)) hoverText = describeMove(mv);
                if (button(x, y, bw, bh, label, mv.damaging() ? rgba(120, 60, 60) : rgba(50, 80, 120), ok, 13)) {
                    pendingMove_ = m;
                    const auto t = battle_->validTargets(0, curSlot_, m);
                    if (t.empty()) {
                        acts_[static_cast<size_t>(curSlot_)] = {gf::Action::UseMove, m, {1, 0}, -1};
                        chosen_[static_cast<size_t>(curSlot_)] = true;
                        battleAdvance();
                    } else if (t.size() == 1) {
                        acts_[static_cast<size_t>(curSlot_)] = {gf::Action::UseMove, m, t[0], -1};
                        chosen_[static_cast<size_t>(curSlot_)] = true;
                        battleAdvance();
                    } else {
                        phase_ = Phase::Target;
                    }
                }
            }
            if (!hoverText.empty()) gx_->text(hoverText, b.x, aY + 26 + 2 * 40 + 2, 520, 34, 11, kDim, TF_WRAP);

            const float rx = b.x + 540, rw = 150;
            if (button(rx, aY + 26, rw, 34, "Switch friend", kBtn, battle_->canSwitch(0, curSlot_))) phase_ = Phase::SwitchPick;
            if (battle_->wild) {
                if (button(rx, aY + 66, rw, 34, "Catch Charm (" + std::to_string(g_.charms) + ")", rgba(100, 80, 170), g_.charms > 0)) phase_ = Phase::CharmTarget;
                if (button(rx + rw + 8, aY + 26, rw, 34, "Run away", rgba(110, 80, 50))) battleFinish(true);
            }
            bool any = false;
            for (bool c : chosen_) any = any || c;
            if (button(rx + rw + 8, aY + 66, rw, 34, "Undo choices", kBtn, any)) { chosen_ = {{false, false, false}}; battleAdvance(); }
        } else {
            gx_->text(phase_ == Phase::CharmTarget ? "Pick a wild friend to charm (click a highlighted card)." : "Pick a target (click a highlighted card).", b.x, aY, 600, 22, 14, kAccent, TF_BOLD);
            if (button(b.x, aY + 30, 120, 34, "Cancel", kBtn)) phase_ = Phase::Choose;
        }
    }

    blocked_ = prevBlocked;

    // ---- Modals ----
    if (modalActive) {
        gx_->fillRect(b.x - 8, b.y - 8, b.w + 16, b.h + 16, rgba(0, 0, 0, 150), 8);
        const float mw = 460;
        std::vector<int> bench = battle_->benchAlive(0);
        const bool isDone = phase_ == Phase::Done;
        const float mh = isDone ? 70.f + static_cast<float>(resultLines_.size()) * 24.f + 60.f : 90.f + static_cast<float>(bench.size()) * 40.f + 50.f;
        const float mx = b.x + (b.w - mw) / 2, my = b.y + 30;
        card(mx, my, mw, mh, rgba(30, 36, 54));
        gx_->strokeRect(mx, my, mw, mh, kAccent, 2, 8);
        if (isDone) {
            const bool won = battle_->winner == 0;
            gx_->text(won ? "Victory!" : (battle_->over() ? "Defeated" : "Battle over"), mx, my + 10, mw, 34, 24, won ? kGreen : kRed, TF_CENTER | TF_BOLD);
            float ly = my + 56;
            for (const auto& l : resultLines_) { gx_->text(l, mx + 20, ly, mw - 40, 22, 13, kText, TF_CENTER); ly += 24; }
            if (button(mx + mw / 2 - 70, my + mh - 54, 140, 38, "Close", kBlue, true, 14, true)) { battle_.reset(); phase_ = Phase::Choose; }
        } else {
            int slot = curSlot_;
            if (phase_ == Phase::Replace) {
                const auto empties = battle_->emptyPlayerSlotsNeedingReplacement();
                if (!empties.empty()) slot = empties[0];
            }
            gx_->text(phase_ == Phase::Replace ? "A friend fainted! Who joins the fight?" : "Switch to which friend?", mx, my + 12, mw, 24, 16, kText, TF_CENTER | TF_BOLD);
            float by = my + 50;
            for (int idx : bench) {
                const gf::Fighter& f = battle_->sides[0].team[static_cast<size_t>(idx)];
                const std::string label = f.sp().name + "   Lv " + std::to_string(f.level) + "   HP " + std::to_string(static_cast<int>(f.hpPct() * 100)) + "%";
                if (button(mx + 40, by, mw - 80, 34, label, kBtn, true, 13, true)) {
                    if (phase_ == Phase::Replace) {
                        battle_->replace(0, slot, idx);
                        if (battle_->emptyPlayerSlotsNeedingReplacement().empty()) battleAdvance();
                    } else {
                        acts_[static_cast<size_t>(curSlot_)] = {gf::Action::Switch, 0, {0, 0}, idx};
                        chosen_[static_cast<size_t>(curSlot_)] = true;
                        battleAdvance();
                    }
                    break;
                }
                by += 40;
            }
            if (phase_ == Phase::SwitchPick && button(mx + mw / 2 - 60, by + 8, 120, 32, "Cancel", rgba(90, 60, 60), true, 13, true)) phase_ = Phase::Choose;
        }
    }
}

// ---------------------------------------------------------------------------
// Guide
// ---------------------------------------------------------------------------

namespace {
struct GuideSection { std::string title; std::vector<std::string> paras; };

const std::vector<GuideSection>& guideSections() {
    static const std::vector<GuideSection> g = [] {
        std::vector<GuideSection> v;
        v.push_back({"Welcome", {
            "# What is Goal Friends?",
            "Real-life tasks power your creature collection. Your friends can't level up by playing alone: finishing tasks and focus sessions is the only way to earn XP, Energy and Coins. You then spend that progress on levelling, hatching new friends and battling.",
            "# The strip",
            "Your friends wander along the bottom of the screen. The buttons on the right open the panels. Click a friend to open their page. A speech bubble over your lead friend shows how many tasks are due.",
            "# Hiding the strip",
            "Press Ctrl+Shift+Ô to hide or show the whole window (or click the tray icon). Reminders keep working while it's hidden. Right-click the tray icon to quit.",
            "# First steps",
            "- Open Tasks, type something you want to get done and press Add task.",
            "- When you really did it, press Done. You get XP, Energy and Coins.",
            "- Spend Energy on battles and Coins in the Shop. Check this guide whenever something is unclear."}});
        v.push_back({"Tasks & rewards", {
            "# Adding tasks",
            "New tasks don't repeat by default. Use Repeat to make a task daily, on chosen weekdays, every N days, weekly or monthly. Use Time to give it a window (morning, afternoon, evening) or a specific time.",
            "# Rewards",
            "- Effort S / M / L / XL gives 10 / 25 / 60 / 120 XP, 1 / 2 / 4 / 8 Energy and 2 / 5 / 12 / 24 Coins.",
            "- The XP goes to your lead friend (the first one in your party).",
            "- Streak bonus: +5% XP per consecutive completion of a recurring task (up to +50%), plus +2% per day of your overall streak (up to +20%).",
            "- Finish a timed task after its window has closed (within 2 hours) and you get half XP. Missing a recurring task completely resets its streak.",
            "# Reminders",
            "Tasks with a time or window send a notification when they start, nudges 30 minutes and 2 hours later, and one near the end of the window. Press Snooze on a task to be reminded in an hour. Between 22:00 and 08:00 reminders are held back.",
            "# Sleepy friends",
            "If a whole day passes with no tasks or focus sessions, your friends get sleepy. They nap on the strip and fight with -20% ATK and SPD until you finish something. Nothing is ever lost."}});
        v.push_back({"Friends & eggs", {
            "# Your team",
            "You bring a party of 6 into battle; every other friend waits in the Box. Open a friend in the Friends tab to move them, or to make them the lead.",
            "# Levels and evolution",
            "Friends level up from 1 to 50. Three-stage lines evolve at levels 16 and 32, two-stage lines at level 20. Rare friends don't evolve. Each friend has exactly four unique moves, and an evolved form gets a new set.",
            "# Stats",
            "Every friend has six stats, like the Pokémon games: HP (health), ATK (attack), DEF (defence), SPA (special attack), SPD (special defence) and SPE (speed). Physical moves use the user's ATK against the target's DEF; special moves use SPA against SPD. Each move says Physical or Special. Speed decides who acts first.",
            "# Getting new friends",
            "- Eggs cost 40 coins in the Shop and hatch after you complete 5 tasks. Some trainers also give you eggs.",
            "- Catch Charms (20 coins) can befriend a wild friend in battle. They work best when the foe is hurt.",
            "- Winning a boss battle always gives you a rare friend.",
            "- A friend you already own is turned into coins instead.",
            "# The Dex",
            "The Dex lists every friend. You can read the moves of every friend you have met or owned."}});
        v.push_back({"Focus sessions", {
            "# How it works",
            "Pick a length (15, 25 or 45 minutes; a 1-minute test is there for trying things out) and start. When the timer ends you earn 1 Energy per 10 minutes, XP for your lead friend equal to the minutes, and it counts as activity for your streak.",
            "# Staying focused",
            "While a session runs the rest of the app is locked: only the Focus tab works. You can cancel, but you get no reward.",
            "# Afterwards",
            "No window pops up. Instead the Battle button on the strip pulses and your lead friend asks if it's battle time. Open Battle whenever you're ready."}});
        v.push_back({"Battles", {
            "# The basics",
            "Each side brings a team of 6; 3 friends fight at once and 3 wait on the bench. Each turn you pick an action for every active friend: use a move, switch to a bench friend, and in wild battles use a Catch Charm or run away.",
            "# How a turn plays out",
            "When everyone has chosen, the turn is replayed step by step: the acting friend glows, health bars move, red numbers show damage and green numbers healing, and the log says what happened. Use Skip to jump to the end or Speed to make it faster.",
            "# Turn order",
            "Switching goes first. Then moves with Priority, then everyone else by speed (SPE). Ties are random.",
            "# Moves",
            "- Light, Medium and Heavy hits do damage, as Physical (ATK vs DEF) or Special (SPA vs SPD) moves. Utility moves do something else: heal, shield, buff, weaken, trap, and more. Hover over a move to read what it does.",
            "- CD n means the move can't be used for n turns after you use it. If all four moves are cooling down, your friend uses Scuffle, a basic attack.",
            "- Most moves hit one foe. Spread moves hit all foes at 75% damage. Some moves hit random foes, and some can target an ally or yourself.",
            "# Winning",
            "Knock out all of the other team to win. Your friends are fully healed afterwards, so losing costs you nothing but the Energy. After turn 30, fatigue drains everyone more each turn so battles always end.",
            "# Costs",
            "A wild encounter costs 1 Energy, a trainer duel 2, and a boss battle 5 (and needs a 3-day streak)."}});
        {
            std::vector<std::string> ab = {"# What abilities are", "Every friend has one passive ability that works on its own during battles, like in the Pokemon games. Evolutions keep their line's ability. Find it on a friend's page in Friends or the Dex.", "# All abilities"};
            for (int i = 1; i < gf::kAbilityCount; ++i) {
                const auto& info = gf::abilityInfo(static_cast<gf::Ab>(i));
                ab.push_back(std::string("- ") + info.name + ": " + info.desc);
            }
            v.push_back({"Abilities", ab});
        }
        v.push_back({"Status effects", {
            "# Damage over time and recovery",
            "- Wound: lose 6% of max HP at the end of each turn.",
            "- Regen: heal 8% of max HP at the end of each turn.",
            "- Doom: after a few turns, take a heavy hit (it never knocks the friend out by itself).",
            "# Slowing and disabling",
            "- Daze: attacks have a 30% chance to miss.",
            "- Slow / Haste: SPE down 30% / up 40%.",
            "- Stun: skips its next action.",
            "- Trap: can't be switched out or forced out.",
            "- Seal: the last move it used can't be used for 2 turns.",
            "- Taunt: can only use damaging moves.",
            "# Defence",
            "- Armor: takes 30% less damage.",
            "- Shield: absorbs a set amount of damage.",
            "- Thorns: attackers take 25% of the damage they deal.",
            "- Evade: avoids the next attack this turn.",
            "- Counter: if hit this turn, strikes back.",
            "# Offence",
            "- Focus: the next attack deals +50%.",
            "- Mark: the next hit it receives deals +50%.",
            "- Empower: the next attacks deal extra damage.",
            "- Priority: acts before normal-speed actions.",
            "- Charge: the move takes two turns; some charge moves dodge in between.",
            "- Pivot: swaps out after hitting. Force-switch: pulls a random bench friend in.",
            "- Cleanse: removes all negative effects."}});
        std::vector<std::string> areas = {"# Where to fight", "Pick an area in the Battle tab. Each area has its own friends, a level bonus and a coin multiplier. New areas unlock as you win more battles."};
        for (const auto& a : gf::allAreas())
            areas.push_back("- " + a.name + ": " + a.desc + "  (foes +" + std::to_string(a.levelBonus) + " levels, coins x" + std::to_string(a.coinMult).substr(0, 3) + (a.unlockWins ? ", unlocks after " + std::to_string(a.unlockWins) + " wins)" : ", open from the start)"));
        v.push_back({"Areas", areas});
        v.push_back({"Tips", {
            "# Doing well",
            "- Switch out friends that are nearly knocked out; their statuses reset and the bench keeps its health.",
            "- Use Cleanse moves against Wound, Daze and Doom, and Spread moves when several foes are weak.",
            "- Stun and Trap are strong but have long cooldowns. Save them for the foe that worries you most.",
            "- Healers and shielders can target allies. Put one on your team and aim their moves at whoever needs them.",
            "- Short on Energy? Do a task or a focus session. Even a small S task gives you 1.",
            "# Keeping the habit",
            "- Add recurring tasks with a time window so reminders nudge you at the right moment.",
            "- A streak is worth protecting: even one small task a day keeps your friends happy and your bonuses growing."}});
        return v;
    }();
    return g;
}
}  // namespace

void Ui::drawGuide(const Rect& b) {
    const auto& secs = guideSections();
    const float nw = 190;
    for (size_t i = 0; i < secs.size(); ++i) {
        if (button(b.x, b.y + static_cast<float>(i) * 40, nw, 34, secs[i].title, guideSection_ == static_cast<int>(i) ? kBlue : kBtn, true, 13)) {
            guideSection_ = static_cast<int>(i);
            scrollGuide_ = 0;
            guideContent_ = 0;
        }
    }
    const float cx = b.x + nw + 14, cw = b.w - nw - 14;
    card(cx, b.y, cw, b.h);
    const GuideSection& sec = secs[static_cast<size_t>(std::clamp(guideSection_, 0, static_cast<int>(secs.size()) - 1))];
    gx_->pushClip(cx, b.y, cw, b.h);
    // Clamp before drawing (using last frame's content height) so nothing is ever drawn scrolled past the content.
    if (hover(cx, b.y, cw, b.h)) scrollGuide_ -= in_->wheel * 48;
    scrollGuide_ = std::clamp(scrollGuide_, 0.f, std::max(0.f, guideContent_ - b.h));
    float y = b.y + 12 - scrollGuide_;
    gx_->text(sec.title, cx + 18, y, cw - 36, 28, 20, kAccent, TF_BOLD);
    y += 38;
    for (const auto& para : sec.paras) {
        if (para.rfind("# ", 0) == 0) {
            y += 6;
            gx_->text(para.substr(2), cx + 18, y, cw - 36, 22, 15, kText, TF_BOLD);
            y += 26;
        } else if (para.rfind("- ", 0) == 0) {
            const std::string t = para.substr(2);
            const float h = gx_->textHeight(t, cw - 64, 13);
            gx_->fillEllipse(cx + 26, y + 9, 2.5f, 2.5f, kAccent);
            gx_->text(t, cx + 38, y, cw - 64, h + 4, 13, rgba(214, 220, 232), TF_WRAP);
            y += h + 6;
        } else {
            const float h = gx_->textHeight(para, cw - 36, 13);
            gx_->text(para, cx + 18, y, cw - 36, h + 4, 13, rgba(214, 220, 232), TF_WRAP);
            y += h + 8;
        }
    }
    guideContent_ = y + scrollGuide_ - b.y + 12;
    scrollGuide_ = std::clamp(scrollGuide_, 0.f, std::max(0.f, guideContent_ - b.h));
    gx_->popClip();
}

}  // namespace ui
