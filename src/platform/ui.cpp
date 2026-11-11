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

std::string effText(const gf::Effect& e) {
    using gf::EK;
    const std::string a = std::to_string(e.a);
    std::string s;
    switch (e.k) {
        case EK::Wound: s = "Wound " + a; break;
        case EK::Daze: s = "Daze " + a; break;
        case EK::Slow: s = "Slow " + a; break;
        case EK::Haste: s = "Haste " + a; break;
        case EK::Armor: s = "Armor " + a; break;
        case EK::Thorns: s = "Thorns " + a; break;
        case EK::Regen: s = "Regen " + a; break;
        case EK::Focus: s = "Focus"; break;
        case EK::Mark: s = "Mark"; break;
        case EK::Trap: s = "Trap " + a; break;
        case EK::Stun: s = "Stun"; break;
        case EK::Taunt: s = "Taunt " + a; break;
        case EK::Doom: s = "Doom " + a; break;
        case EK::Seal: s = "Seal"; break;
        case EK::Shield: s = "Shield " + a + "%"; break;
        case EK::Heal: s = "Heal " + a + "%"; break;
        case EK::Cleanse: s = "Cleanse"; break;
        case EK::ForceSwitch: s = "Force switch"; break;
        case EK::AtkMod: s = "ATK " + std::string(e.a > 0 ? "+" : "") + a + "% " + std::to_string(e.b) + "t"; break;
        case EK::DefMod: s = "DEF " + std::string(e.a > 0 ? "+" : "") + a + "% " + std::to_string(e.b) + "t"; break;
        case EK::SpdMod: s = "SPD " + std::string(e.a > 0 ? "+" : "") + a + "% " + std::to_string(e.b) + "t"; break;
        case EK::Evade: s = e.chance < 100 ? "Evade (" + std::to_string(e.chance) + "%)" : "Evade"; break;
        case EK::Counter: s = "Counter x" + std::to_string(e.a / 10.0).substr(0, 3); break;
        case EK::CounterHit: s = "Counter hit"; break;
        case EK::CounterAbsorb: s = "Absorb & heal"; break;
        case EK::Empower: s = "Next " + std::to_string(e.b) + " atk +" + a + "%"; break;
        case EK::NoMiss: s = "Can't miss"; break;
        case EK::PrioNext: s = "Next move Priority"; break;
        case EK::SlowAttackers: s = "Slows attackers"; break;
        case EK::ReflectDebuff: s = "Reflect debuff"; break;
        case EK::ReflectDamage: s = "Reflect hit"; break;
        case EK::SelfDamage: s = "Lose " + a + "% HP"; break;
        case EK::Plate: s = "+1 Plate"; break;
        case EK::Leech: s = "Leech"; break;
    }
    if (e.chance < 100 && e.k != gf::EK::Evade) s += " (" + std::to_string(e.chance) + "%)";
    if (e.self) s += " [self]";
    return s;
}

std::string spText(gf::Sp sp) {
    using gf::Sp;
    switch (sp) {
        case Sp::SlipstreamBonus: return "+30% if you move first";
        case Sp::WardenBonus: return "+50% with Armor/Shield/Thorns";
        case Sp::BackdraftBonus: return "+50% vs Wounded";
        case Sp::CannonballBonus: return "+10% per Plate";
        case Sp::ShapeshiftSlam: return "1.2x damage you last took";
        case Sp::Mimic: return "Copy foe's last move";
        case Sp::InvertBuffs: return "Foe's buffs become debuffs";
        case Sp::StealBuff: return "Steal a buff";
        case Sp::CopyMods: return "Copy foe's stat changes";
        case Sp::ResetCooldowns: return "Reset your cooldowns";
        case Sp::WildCard: return "Random effect";
        case Sp::ChaosBolt: return "Random bonus effect";
        case Sp::PrismaticShatter: return "Strip buffs, +20% each";
        case Sp::Anarchy: return "Reset all statuses & cooldowns";
        case Sp::RebirthPyre: return "Revive once when KO'd";
        default: return "";
    }
}

std::string describeMove(const gf::Move& m) {
    std::string s;
    auto add = [&](const std::string& p) { if (!p.empty()) { if (!s.empty()) s += " · "; s += p; } };
    if (m.damaging() || m.sp == gf::Sp::ShapeshiftSlam) {
        if (m.multiHit()) {
            std::string h = std::to_string(m.hits.size()) + " hits";
            if (m.randomTargets) h += " (random foes)";
            add(h);
        } else {
            const int p = m.hits.empty() ? m.power : m.hits[0];
            add(p <= 45 ? "Light hit" : p <= 80 ? "Medium hit" : p > 0 ? "Heavy hit" : "Special hit");
        }
    } else {
        add("Utility");
    }
    if (m.tgt == gf::Tgt::Spread) add("all foes");
    if (m.tgt == gf::Tgt::Ally) add("ally or self");
    if (m.prio) add("Priority");
    if (m.charge) add(m.chargeEvade ? "2 turns (dodges)" : "2 turns");
    if (m.pivot) add("then swap out");
    if (m.ignoreDef) add("ignores " + std::to_string(m.ignoreDef) + "% DEF");
    if (m.ignoreDefenses) add("ignores Shield/Armor");
    if (m.crit) add(std::to_string(m.crit) + "% crit");
    if (m.drain) add("drain " + std::to_string(m.drain) + "%");
    if (m.recoil) add("recoil " + std::to_string(m.recoil) + "%");
    for (const auto& e : m.fx) add(effText(e));
    add(spText(m.sp));
    if (m.once) add("once per battle");
    if (m.cd > 0) add("CD " + std::to_string(m.cd));
    return s;
}

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

void Ui::frame(gfx::Gfx& gx, float W, float H, Input& in, double dt, long long nowMs, tu::Time now) {
    gx_ = &gx; in_ = &in; W_ = W; H_ = H; nowMs_ = nowMs; now_ = now;
    consumed_ = false; blocked_ = false; fieldClicked_ = false;
    animT_ += static_cast<float>(dt);
    if (nagSeconds_ > 0) nagSeconds_ -= static_cast<float>(dt);

    if (!g_.started && tab_ == Tab::None) tab_ = Tab::Start;

    // Focus timer completion: battles open at the end of a focus session.
    if (focusRunning_ && nowMs_ >= focusEndMs_) {
        focusRunning_ = false;
        const gf::Result r = gf::completeFocus(g_, focusMinutes_, now_);
        for (const auto& m : r.messages) toast(m);
        balloon("Focus session complete! Time for a battle?");
        focusDone_ = focusMinutes_;
        dirty_ = true;
        tab_ = Tab::Battle;
    }
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

void Ui::drawWorld(float dt) {
    const float gh = 54.f, gy = H_ - gh;
    const double hour = now_.minute / 60.0;
    // Day/night tint for the ground.
    gfx::Color ground = rgba(74, 150, 86), grass = rgba(110, 190, 100);
    if (hour < 6 || hour >= 20) { ground = rgba(30, 44, 70); grass = rgba(44, 66, 100); }
    else if (hour < 8 || hour >= 18) { ground = rgba(110, 100, 90); grass = rgba(150, 140, 100); }
    ground.a = 0.93f;
    gx_->fillRect(0, gy, W_, gh, ground);
    for (float x = 0; x < W_; x += 9) {
        const float hgt = 3.f + static_cast<float>(static_cast<int>(x * 7) % 3);
        gx_->fillRect(x, gy - hgt + 1, 5, hgt, grass);
    }
    gx_->fillRect(0, gy, W_, 3, grass);

    // Walkers follow the party.
    std::vector<int> ids = g_.party;
    walkers_.erase(std::remove_if(walkers_.begin(), walkers_.end(), [&](const Walker& w) { return std::find(ids.begin(), ids.end(), w.friendId) == ids.end(); }), walkers_.end());
    for (int id : ids) {
        if (std::none_of(walkers_.begin(), walkers_.end(), [&](const Walker& w) { return w.friendId == id; })) {
            Walker w;
            w.friendId = id;
            w.x = 120.f + static_cast<float>((id * 137) % 600);
            w.dir = (id % 2) ? 1.f : -1.f;
            w.speed = 18.f + static_cast<float>((id * 7) % 20);
            walkers_.push_back(w);
        }
    }
    const bool sleepy = gf::isSleepy(g_, now_.day);
    const int lead = gf::leadFriendId(g_);
    for (auto& w : walkers_) {
        const gf::Friend* f = gf::findFriend(g_, w.friendId);
        if (!f) continue;
        const gf::Species& sp = gf::species(f->species);
        const float sc = 2.6f + 0.4f * static_cast<float>(sp.stage);
        const float size = 16 * sc;
        if (!sleepy) {
            if (w.pause > 0) w.pause -= dt;
            else {
                w.x += w.dir * w.speed * dt;
                if (w.x < 30) { w.x = 30; w.dir = 1; }
                if (w.x > W_ - 40) { w.x = W_ - 40; w.dir = -1; }
                if (std::fmod(animT_ + w.friendId, 7.f) < dt) { w.pause = 1.5f + static_cast<float>(w.friendId % 3); }
                if (std::fmod(animT_ * 1.3f + w.friendId * 3, 11.f) < dt) w.dir = -w.dir;
            }
        }
        if (w.hop > 0 || w.hopV != 0) {
            w.hopV -= 600 * dt;
            w.hop += w.hopV * dt;
            if (w.hop <= 0) { w.hop = 0; w.hopV = 0; }
        }
        const int frame = sleepy ? 1 : static_cast<int>(animT_ * 2.5f + w.friendId) % 2;
        const float bob = sleepy ? 0.f : (frame ? 1.5f : 0.f);
        const float sx = w.x - size / 2, sy = gy + 14 - size - bob - w.hop;
        gx_->fillEllipse(w.x, gy + 13, size * 0.38f, 3.5f, rgba(0, 0, 0, 70));
        gx_->sprite(gfx::SpriteKind::Species, f->species, frame, w.dir < 0, sx, sy, sc);
        if (sleepy) gx_->text("z z", w.x - 10, sy - 6, 40, 16, 12, kText, TF_BOLD);
        if (!blocked_ && !consumed_ && in_->pressed && hover(sx, sy, size, size) && !(panel_.w > 0 && tab_ != Tab::None && hover(panel_.x, panel_.y, panel_.w, panel_.h))) {
            consumed_ = true;
            w.hopV = 190;
            w.hop = 0.01f;
            selFriend_ = w.friendId;
            tab_ = Tab::Friends;
        }
        // Nag bubble on the lead friend when something is due.
        if (w.friendId == lead && tab_ == Tab::None) {
            int due = 0;
            for (const auto& o : gf::occurrencesOn(g_, now_.day, now_.minute))
                if (!o.done && !o.missed && now_.minute >= o.win.start) ++due;
            if (due > 0 || nagSeconds_ > 0) {
                const std::string t = due > 0 ? std::to_string(due) + " to do!" : "Hey!";
                const float bw = gx_->textWidth(t, 12, true) + 18;
                const float by = sy - 26 + (std::sin(animT_ * 4) * 2);
                gx_->fillRect(w.x - bw / 2, by, bw, 22, rgba(255, 255, 255, 235), 11);
                gx_->text(t, w.x - bw / 2, by, bw, 22, 12, rgba(30, 30, 40), TF_CENTER | TF_VCENTER | TF_BOLD);
            }
        }
    }

    // HUD and buttons in the ground band.
    std::string hud = "Energy " + std::to_string(g_.energy) + "   Coins " + std::to_string(g_.coins) + "   Streak " + std::to_string(g_.globalStreak) + "d";
    if (!g_.eggs.empty()) hud += "   Eggs " + std::to_string(g_.eggs.size());
    gx_->text(hud, 14, gy + 6, 520, 20, 13, rgba(255, 255, 255), TF_BOLD);
    if (focusRunning_) {
        const long long left = std::max(0LL, (focusEndMs_ - nowMs_) / 1000);
        gx_->text("Focus " + two(static_cast<int>(left / 60)) + ":" + two(static_cast<int>(left % 60)), 14, gy + 28, 300, 20, 13, kAccent, TF_BOLD);
    } else if (sleepy) {
        gx_->text("Your friends are sleepy. Finish a task to wake them!", 14, gy + 28, 420, 20, 12, rgba(255, 255, 255, 220));
    }

    struct TabBtn { Tab t; const char* label; };
    const TabBtn tabs[] = {{Tab::Tasks, "Tasks"}, {Tab::Friends, "Friends"}, {Tab::Dex, "Dex"}, {Tab::Shop, "Shop"}, {Tab::Focus, "Focus"}, {Tab::Battle, "Battle"}};
    const float bw = 78, bgap = 6;
    float bx = W_ - (bw + bgap) * 6 - 6;
    for (const auto& tb : tabs) {
        std::string label = tb.label;
        if (tb.t == Tab::Tasks) {
            int due = 0;
            for (const auto& o : gf::occurrencesOn(g_, now_.day, now_.minute)) if (!o.done && !o.missed) ++due;
            if (due > 0) label += " (" + std::to_string(due) + ")";
        }
        const bool active = tab_ == tb.t;
        if (button(bx, gy + 12, bw, 30, label, active ? kAccent : rgba(40, 48, 70, 230), g_.started, 12) && g_.started) tab_ = active ? Tab::None : tb.t;
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
        const T tabs[] = {{Tab::Tasks, "Tasks"}, {Tab::Friends, "Friends"}, {Tab::Dex, "Dex"}, {Tab::Shop, "Shop"}, {Tab::Focus, "Focus"}, {Tab::Battle, "Battle"}};
        float x = panel_.x + 12;
        for (const auto& t : tabs) {
            if (button(x, 10, 96, 30, t.n, tab_ == t.t ? kBlue : kBtn, true, 13)) tab_ = t.t;
            x += 102;
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
        gx_->sprite(gfx::SpriteKind::Species, id, static_cast<int>(animT_ * 2) % 2, false, x + (cw - 128) / 2, 126, 8);
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
        gx_->sprite(gfx::SpriteKind::Species, f.species, static_cast<int>(animT_ * 2 + static_cast<float>(i)) % 2, false, x + 8, y + 18, 4);
        gx_->text(sp.name, x + 80, y + 6, cw - 84, 20, 13, kText, TF_BOLD);
        gx_->text("Lv " + std::to_string(f.level), x + 80, y + 28, 80, 18, 12, kAccent, TF_BOLD);
        if (f.level < gf::kMaxLevel) bar(x + 80, y + 52, cw - 92, 8, static_cast<double>(f.xp) / gf::xpToNext(f.level), kBlue);
        gx_->text(inParty ? (g_.party[0] == f.id ? "Party (lead)" : "Party") : "Box", x + 80, y + 66, cw - 84, 18, 11, inParty ? kGreen : kDim);
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
    gx_->sprite(gfx::SpriteKind::Species, sf->species, static_cast<int>(animT_ * 2) % 2, false, dx + 10, b.y + 10, 5);
    gx_->text(sp.name, dx + 100, b.y + 10, dw - 110, 26, 18, kText, TF_BOLD);
    gx_->text("Stage " + std::to_string(sp.stage) + (sp.rare ? " · Rare" : "") + " · Lv " + std::to_string(sf->level), dx + 100, b.y + 38, dw - 110, 18, 12, kDim);
    if (sp.evolvesTo >= 0) gx_->text("Evolves at Lv " + std::to_string(sp.evolveLevel), dx + 100, b.y + 56, dw - 110, 18, 12, kAccent);
    gx_->text("HP " + std::to_string(st.hp) + "  ATK " + std::to_string(st.atk) + "  DEF " + std::to_string(st.def) + "  SPD " + std::to_string(st.spd), dx + 10, b.y + 96, dw - 20, 18, 12, kText, TF_BOLD);
    float my = b.y + 120;
    for (const auto& m : sp.moves) {
        gx_->text(m.name, dx + 10, my, dw - 20, 18, 12, kAccent, TF_BOLD);
        gx_->text(describeMove(m), dx + 10, my + 17, dw - 20, 30, 11, kDim, TF_WRAP);
        my += 52;
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
        gx_->sprite(gfx::SpriteKind::Egg, 0, static_cast<int>(animT_ * 2) % 2, false, ex, by + 30, 1.5f);
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
    const float gy0 = b.y + 24;
    for (int i = 0; i < gf::kSpeciesCount; ++i) {
        const float x = b.x + static_cast<float>(i % cols) * (cw + gap), y = gy0 + static_cast<float>(i / cols) * (ch + gap);
        const bool own = g_.dexOwned.count(i) > 0, seen = own || g_.dexSeen.count(i) > 0;
        card(x, y, cw, ch, i == selDex_ ? kCard2 : kCard);
        if (i == selDex_) gx_->strokeRect(x, y, cw, ch, kAccent, 2, 8);
        gx_->sprite(seen ? gfx::SpriteKind::Species : gfx::SpriteKind::Silhouette, i, 0, false, x + (cw - 40) / 2, y + 6, 2.5f);
        gx_->text(seen ? gf::species(i).name : "???", x, y + 52, cw, 18, 11, own ? kText : kDim, TF_CENTER | TF_BOLD);
        gx_->text(own ? "owned" : seen ? "seen" : "", x, y + 66, cw, 16, 10, own ? kGreen : kDim, TF_CENTER);
        if (hover(x, y, cw, ch) && in_->pressed && !consumed_ && !blocked_) { consumed_ = true; selDex_ = i; }
    }
    const float dx = b.x + cols * (cw + gap) + 14, dw = b.w - cols * (cw + gap) - 14;
    if (selDex_ >= 0) {
        const bool seen = g_.dexOwned.count(selDex_) || g_.dexSeen.count(selDex_);
        card(dx, b.y, dw, b.h);
        if (!seen) {
            gx_->sprite(gfx::SpriteKind::Silhouette, selDex_, 0, false, dx + 12, b.y + 12, 5);
            gx_->text("???", dx + 100, b.y + 24, dw - 110, 26, 18, kDim, TF_BOLD);
            gx_->text("Find this friend by hatching eggs, battling, or catching wild ones.", dx + 12, b.y + 110, dw - 24, 60, 12, kDim, TF_WRAP);
        } else {
            const gf::Species& sp = gf::species(selDex_);
            gx_->sprite(gfx::SpriteKind::Species, selDex_, static_cast<int>(animT_ * 2) % 2, false, dx + 12, b.y + 12, 5);
            gx_->text(sp.name, dx + 100, b.y + 14, dw - 110, 26, 18, kText, TF_BOLD);
            gx_->text("Stage " + std::to_string(sp.stage) + (sp.rare ? " · Rare" : ""), dx + 100, b.y + 42, dw - 110, 18, 12, kDim);
            gx_->text("Base  HP " + std::string(static_cast<size_t>(sp.hp), '|') + "  ATK " + std::string(static_cast<size_t>(sp.atk), '|'), dx + 12, b.y + 98, dw - 24, 16, 11, kText);
            gx_->text("DEF " + std::string(static_cast<size_t>(sp.def), '|') + "  SPD " + std::string(static_cast<size_t>(sp.spd), '|'), dx + 12, b.y + 114, dw - 24, 16, 11, kText);
            float my = b.y + 138;
            for (const auto& m : sp.moves) {
                gx_->text(m.name, dx + 12, my, dw - 24, 18, 12, kAccent, TF_BOLD);
                gx_->text(describeMove(m), dx + 12, my + 17, dw - 24, 30, 11, kDim, TF_WRAP);
                my += 50;
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
    gx_->sprite(gfx::SpriteKind::Egg, 0, static_cast<int>(animT_ * 2) % 2, false, x0 + (cw - 96) / 2, y0 + 16, 6);
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
    gx_->text("Work without distractions. When the timer ends you earn Energy, XP for your lead, and a chance to battle.", b.x + 100, b.y + 36, b.w - 200, 40, 13, kDim, TF_CENTER | TF_WRAP);
    if (focusRunning_) {
        const long long left = std::max(0LL, (focusEndMs_ - nowMs_) / 1000);
        const double frac = 1.0 - static_cast<double>(left) / (focusMinutes_ * 60.0);
        gx_->text(two(static_cast<int>(left / 60)) + ":" + two(static_cast<int>(left % 60)), b.x, b.y + 110, b.w, 90, 72, kText, TF_CENTER | TF_BOLD | TF_VCENTER);
        bar(cx - 250, b.y + 220, 500, 14, frac, kGreen);
        gx_->text("Stay focused. Your friends are cheering you on!", b.x, b.y + 244, b.w, 24, 13, kDim, TF_CENTER);
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
    if (!gf::startBattle(g_, m, now_, bt, why)) { toast(why); return; }
    battle_ = std::move(bt);
    mode_ = m;
    phase_ = Phase::Choose;
    chosen_ = {{false, false, false}};
    curSlot_ = 0;
    pendingMove_ = -1;
    resultLines_.clear();
    focusDone_ = 0;
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
    battle_->resolveTurn(acts_);
    chosen_ = {{false, false, false}};
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

void Ui::fighterCard(float x, float y, float w, float h, int side, int slot, bool highlight, bool targetable, bool* clicked) {
    if (clicked) *clicked = false;
    if (!battle_ || !battle_->slotFilled(side, slot)) {
        card(x, y, w, h, rgba(26, 30, 42));
        gx_->text("-", x, y, w, h, 20, kDim, TF_CENTER | TF_VCENTER);
        return;
    }
    const gf::Fighter& f = battle_->fighter(side, slot);
    const bool dead = !f.alive();
    gx_->fillRect(x, y, w, h, side == 1 ? rgba(52, 34, 40) : rgba(30, 42, 60), 8);
    if (highlight) gx_->strokeRect(x, y, w, h, kAccent, 2.5f, 8);
    if (targetable) {
        const bool hv = hover(x, y, w, h);
        gx_->strokeRect(x, y, w, h, hv ? rgba(255, 255, 255) : kGreen, hv ? 3.f : 2.f, 8);
        if (hv && in_->pressed && !consumed_) { consumed_ = true; if (clicked) *clicked = true; }
    }
    gx_->sprite(gfx::SpriteKind::Species, f.speciesId, static_cast<int>(animT_ * 2 + static_cast<float>(slot)) % 2, side == 1, x + 8, y + 18, 4, dead ? 0.25f : 1.f);
    gx_->text(battle_->nameOf(side, slot) + "  Lv " + std::to_string(f.level), x + 80, y + 6, w - 86, 20, 13, kText, TF_BOLD);
    bar(x + 80, y + 30, w - 92, 10, f.hpPct(), f.hpPct() > 0.5 ? kGreen : f.hpPct() > 0.2 ? rgba(220, 170, 40) : kRed);
    gx_->text(std::to_string(f.hp) + " / " + std::to_string(f.maxHp), x + 80, y + 42, w - 92, 16, 11, kDim);
    gx_->text(statusText(f), x + 80, y + 60, w - 90, 40, 10, rgba(255, 220, 140), TF_WRAP);
}

void Ui::drawBattle(const Rect& b) {
    // ---- No battle running: mode selection ----
    if (!battle_) {
        if (focusDone_ > 0) {
            card(b.x, b.y, b.w, 46, rgba(36, 60, 44));
            gx_->text("Focus session complete! You earned Energy. Choose a battle below.", b.x, b.y, b.w, 46, 15, kGreen, TF_CENTER | TF_VCENTER | TF_BOLD);
        }
        gx_->text("Energy: " + std::to_string(g_.energy) + "     Party: " + std::to_string(g_.party.size()) + " friend(s)", b.x, b.y + 54, b.w, 24, 15, kAccent, TF_CENTER | TF_BOLD);
        struct Mode { gf::BattleMode m; const char* name; const char* desc; };
        const Mode modes[] = {
            {gf::BattleMode::Wild, "Wild encounter", "1 to 3 wild friends. Use a Catch Charm to befriend one."},
            {gf::BattleMode::Duel, "Trainer duel", "A full team of foes. Win coins and maybe an Egg."},
            {gf::BattleMode::Boss, "Boss battle", "A powerful rare-friend team. Needs a 3-day streak. Guaranteed rare reward."}};
        const float cw = 300, gap = 20, x0 = b.x + (b.w - (cw * 3 + gap * 2)) / 2, y0 = b.y + 94;
        for (int i = 0; i < 3; ++i) {
            const float x = x0 + static_cast<float>(i) * (cw + gap);
            card(x, y0, cw, 250);
            gx_->text(modes[i].name, x, y0 + 14, cw, 26, 18, kText, TF_CENTER | TF_BOLD);
            gx_->text(std::to_string(gf::energyCost(modes[i].m)) + " Energy", x, y0 + 46, cw, 22, 15, kAccent, TF_CENTER | TF_BOLD);
            gx_->text(modes[i].desc, x + 20, y0 + 84, cw - 40, 70, 13, kDim, TF_CENTER | TF_WRAP);
            const bool locked = modes[i].m == gf::BattleMode::Boss && !gf::bossUnlocked(g_);
            const bool afford = g_.energy >= gf::energyCost(modes[i].m);
            if (locked) gx_->text("Locked: streak " + std::to_string(g_.globalStreak) + " / 3 days", x, y0 + 170, cw, 20, 12, rgba(230, 140, 140), TF_CENTER);
            else if (!afford) gx_->text("Not enough Energy", x, y0 + 170, cw, 20, 12, rgba(230, 140, 140), TF_CENTER);
            if (button(x + 70, y0 + 196, cw - 140, 38, "Fight!", kRed, !locked && afford, 15)) beginBattle(modes[i].m);
        }
        gx_->text("Earn Energy by finishing tasks and focus sessions. Friends are fully healed after every battle.", b.x, b.y + b.h - 28, b.w, 22, 12, kDim, TF_CENTER);
        return;
    }

    // ---- Active battle ----
    const float cardW = (b.w - 20) / 3, cardH = 104;
    const float logY = b.y + cardH + 6, logH = 62, pY = logY + logH + 6, aY = pY + cardH + 6;
    const bool modalActive = phase_ == Phase::SwitchPick || phase_ == Phase::Replace || phase_ == Phase::Done;
    const bool prevBlocked = blocked_;
    if (modalActive) blocked_ = true;

    std::vector<gf::Ref> targets;
    if (phase_ == Phase::Target && battle_->slotFilled(0, curSlot_)) targets = battle_->validTargets(0, curSlot_, pendingMove_);
    auto isTarget = [&](int side, int slot) {
        if (phase_ == Phase::CharmTarget) return side == 1 && battle_->slotFilled(1, slot) && battle_->fighter(1, slot).alive();
        for (const auto& t : targets) if (t.side == side && t.slot == slot) return true;
        return false;
    };

    for (int k = 0; k < 3; ++k) {
        bool clicked = false;
        const float x = b.x + static_cast<float>(k) * (cardW + 10);
        fighterCard(x, b.y, cardW, cardH, 1, k, false, isTarget(1, k), &clicked);
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
    gx_->text("Foes bench: " + std::to_string(battle_->benchAlive(1).size()), b.x + b.w - 150, b.y + cardH - 18, 146, 16, 10, kDim, TF_RIGHT);

    // Log
    card(b.x, logY, b.w, logH, rgba(14, 17, 26));
    const auto& lg = battle_->log;
    const size_t start = lg.size() > 4 ? lg.size() - 4 : 0;
    for (size_t i = start; i < lg.size(); ++i)
        gx_->text(lg[i], b.x + 12, logY + 3 + static_cast<float>(i - start) * 14.5f, b.w - 24, 16, 11, i + 1 == lg.size() ? kText : kDim);

    for (int k = 0; k < 3; ++k) {
        bool clicked = false;
        const float x = b.x + static_cast<float>(k) * (cardW + 10);
        const bool cur = phase_ != Phase::Done && k == curSlot_ && (phase_ == Phase::Choose || phase_ == Phase::Target || phase_ == Phase::SwitchPick);
        fighterCard(x, pY, cardW, cardH, 0, k, cur, isTarget(0, k), &clicked);
        if (clicked) {
            acts_[static_cast<size_t>(curSlot_)] = {gf::Action::UseMove, pendingMove_, {0, k}, -1};
            chosen_[static_cast<size_t>(curSlot_)] = true;
            battleAdvance();
        }
    }
    gx_->text("Your bench: " + std::to_string(battle_->benchAlive(0).size()) + "   Turn " + std::to_string(battle_->turn + 1), b.x + b.w - 250, pY + cardH - 18, 246, 16, 10, kDim, TF_RIGHT);

    // Action area
    if ((phase_ == Phase::Choose || phase_ == Phase::Target || phase_ == Phase::CharmTarget) && battle_->slotFilled(0, curSlot_)) {
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
    if (phase_ == Phase::SwitchPick || phase_ == Phase::Replace || phase_ == Phase::Done) {
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

}  // namespace ui
