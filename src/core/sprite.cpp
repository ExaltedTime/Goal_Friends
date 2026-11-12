#include "sprite.hpp"

#include <algorithm>
#include <cmath>

#include "data.hpp"
#include "sprite_gen.hpp"

namespace gf {
namespace {

using u32 = uint32_t;
constexpr int N = kSpriteSize;

u32 rgb(int r, int g, int b) { return 0xFF000000u | (static_cast<u32>(std::clamp(r, 0, 255)) << 16) | (static_cast<u32>(std::clamp(g, 0, 255)) << 8) | static_cast<u32>(std::clamp(b, 0, 255)); }
int R(u32 c) { return static_cast<int>((c >> 16) & 255); }
int G(u32 c) { return static_cast<int>((c >> 8) & 255); }
int B(u32 c) { return static_cast<int>(c & 255); }
u32 mul(u32 c, double f) { return rgb(static_cast<int>(R(c) * f), static_cast<int>(G(c) * f), static_cast<int>(B(c) * f)); }
u32 mix(u32 a, u32 b, double t) { return rgb(static_cast<int>(R(a) + (R(b) - R(a)) * t), static_cast<int>(G(a) + (G(b) - G(a)) * t), static_cast<int>(B(a) + (B(b) - B(a)) * t)); }

struct Pal { u32 body, belly, accent, accent2; };

struct Cv {
    std::vector<u32> p = std::vector<u32>(N * N, 0);

    static bool in(int x, int y) { return x >= 0 && y >= 0 && x < N && y < N; }
    void set(int x, int y, u32 c) { if (in(x, y)) p[static_cast<size_t>(y * N + x)] = c; }
    u32 get(int x, int y) const { return in(x, y) ? p[static_cast<size_t>(y * N + x)] : 0; }
    void m(int x, int y, u32 c) { set(x, y, c); set(N - 1 - x, y, c); }        // mirrored pixel
    void rect(int x0, int y0, int x1, int y1, u32 c) { for (int y = y0; y <= y1; ++y) for (int x = x0; x <= x1; ++x) set(x, y, c); }
    void mrect(int x0, int y0, int x1, int y1, u32 c) { for (int y = y0; y <= y1; ++y) for (int x = x0; x <= x1; ++x) m(x, y, c); }

    void ell(double cx, double cy, double rx, double ry, u32 c, bool shade = true) {
        for (int y = static_cast<int>(std::floor(cy - ry)) - 1; y <= static_cast<int>(std::ceil(cy + ry)) + 1; ++y)
            for (int x = static_cast<int>(std::floor(cx - rx)) - 1; x <= static_cast<int>(std::ceil(cx + rx)) + 1; ++x) {
                const double dx = (x + 0.5 - cx) / rx, dy = (y + 0.5 - cy) / ry;
                if (dx * dx + dy * dy > 1.0) continue;
                double f = 1.0;
                if (shade) {
                    const double t = std::clamp((y + 0.5 - (cy - ry)) / (2 * ry), 0.0, 1.0);
                    f = 1.12 - 0.34 * t;
                    if (dx < -0.35 && dy < -0.15) f += 0.07;
                }
                set(x, y, mul(c, f));
            }
    }
    void line(int x0, int y0, int x1, int y1, u32 c) {
        const int dx = std::abs(x1 - x0), dy = -std::abs(y1 - y0), sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1;
        int err = dx + dy;
        for (;;) {
            set(x0, y0, c);
            if (x0 == x1 && y0 == y1) break;
            const int e2 = 2 * err;
            if (e2 >= dy) { err += dy; x0 += sx; }
            if (e2 <= dx) { err += dx; y0 += sy; }
        }
    }
    void mline(int x0, int y0, int x1, int y1, u32 c) { line(x0, y0, x1, y1, c); line(N - 1 - x0, y0, N - 1 - x1, y1, c); }
    void tri(double x0, double y0, double x1, double y1, double x2, double y2, u32 c) {
        const int minx = static_cast<int>(std::floor(std::min({x0, x1, x2}))), maxx = static_cast<int>(std::ceil(std::max({x0, x1, x2})));
        const int miny = static_cast<int>(std::floor(std::min({y0, y1, y2}))), maxy = static_cast<int>(std::ceil(std::max({y0, y1, y2})));
        auto edge = [](double ax, double ay, double bx, double by, double px, double py) { return (bx - ax) * (py - ay) - (by - ay) * (px - ax); };
        for (int y = miny; y <= maxy; ++y)
            for (int x = minx; x <= maxx; ++x) {
                const double px = x + 0.5, py = y + 0.5;
                const double a = edge(x0, y0, x1, y1, px, py), b = edge(x1, y1, x2, y2, px, py), d = edge(x2, y2, x0, y0, px, py);
                if ((a >= 0 && b >= 0 && d >= 0) || (a <= 0 && b <= 0 && d <= 0)) set(x, y, c);
            }
    }
    void mtri(double x0, double y0, double x1, double y1, double x2, double y2, u32 c) {
        tri(x0, y0, x1, y1, x2, y2, c);
        tri(N - x0, y0, N - x1, y1, N - x2, y2, c);
    }
    void dot(double cx, double cy, double r, u32 c, bool shade = false) { ell(cx, cy, r, r, c, shade); }
};

// Face: 2x2 eyes with a pupil. style 0 friendly, 1 sleepy/blink, 2 fierce, 3 glowing.
void eyes(Cv& c, int a, int y, int style, u32 glow = 0) {
    const u32 white = rgb(250, 250, 255), ink = rgb(25, 20, 35);
    if (style == 1) { c.m(a, y + 1, ink); c.m(a + 1, y + 1, ink); return; }
    if (style == 3) { c.m(a, y, glow); c.m(a + 1, y, glow); c.m(a, y + 1, glow); c.m(a + 1, y + 1, mix(glow, rgb(255, 255, 255), 0.6)); return; }
    c.m(a, y, white); c.m(a + 1, y, white); c.m(a, y + 1, white); c.m(a + 1, y + 1, ink);
    if (style == 2) { c.m(a, y - 1, ink); c.m(a + 1, y - 1, ink); }
}

void feet(Cv& c, int y, u32 col, int f, int gap = 5) {
    c.mrect(12 - gap - 1 + (f ? 1 : 0), y, 12 - gap + 1 + (f ? 1 : 0), y + 1, col);
}

// ---------------------------------------------------------------------------
// One painter per line. Back layers first, then the body, then front details.
// ---------------------------------------------------------------------------

// 0: Stalwart tank - mossy body, sprout crown, thorns (regrowth, Thorns, Armor)
void paintMoss(Cv& c, int stage, int f, const Pal& P, int& eyeY, int& eyeA) {
    const u32 bark = P.accent2, leaf = P.accent;
    const double ry = 4.2 + stage * 1.25, cy = 16.2 - (stage - 1) * 0.7;
    feet(c, 20, mul(P.body, 0.8), f, 4 + stage);
    c.ell(12, cy, 4.6 + stage * 1.35, ry, stage == 3 ? mix(P.body, bark, 0.5) : P.body);
    c.ell(12, cy + 1.5, 3.2 + stage * 0.4, 2.4 + stage * 0.4, P.belly);
    // thorns along the shoulders
    for (int i = 0; i < stage + 1; ++i) c.mtri(6 - i * 0.2, 11 + i * 2.2, 3.2 - i * 0.4, 10.3 + i * 2.2, 5.8 - i * 0.2, 13.2 + i * 2.2, bark);
    // sprout / crown
    c.rect(11, 6 - stage, 12, 9, bark);
    const int lf = f ? 1 : 0;
    c.ell(8.6, 6.3 - stage + lf, 2.6, 1.5, leaf, false); c.ell(15.4, 6.3 - stage + lf, 2.6, 1.5, leaf, false);
    if (stage >= 2) { c.ell(9.5, 3.8 - stage / 2 + lf, 2.2, 1.3, mul(leaf, 1.15), false); c.ell(14.5, 3.8 - stage / 2 + lf, 2.2, 1.3, mul(leaf, 1.15), false); }
    if (stage == 3) { c.ell(12, 3.3, 3, 1.6, mul(leaf, 1.25), false); c.m(7, 14, mul(bark, 0.8)); c.m(6, 17, mul(bark, 0.8)); c.line(9, 12, 9, 18, mul(bark, 0.7)); }
    eyeY = static_cast<int>(cy) - 1; eyeA = 8;
}

// 1: Swift fox - big ears, lightning tail and cheek marks (Haste, Priority, Evade)
void paintFox(Cv& c, int stage, int f, const Pal& P, int& eyeY, int& eyeA) {
    const u32 bolt = P.accent;
    // zigzag lightning tail
    const int tx = f ? 1 : 0;
    c.line(18 + tx, 18, 21, 15, bolt); c.line(21, 15, 19 + tx, 13, bolt); c.line(19 + tx, 13, 22, 9, bolt);
    c.line(19 + tx, 18, 22, 15, mul(bolt, 0.8));
    if (stage >= 2) c.line(20, 12, 17, 12, bolt);
    const double ry = 4.0 + stage * 1.0, cy = 16.4 - (stage - 1) * 0.6;
    feet(c, 20, mul(P.body, 0.75), f, 3 + stage);
    if (stage == 3) { c.ell(12, cy - 3, 7.5, 3.5, rgb(235, 240, 250), false); c.ell(12, cy - 3.6, 6, 2.4, rgb(250, 252, 255), false); }  // storm-cloud mane
    c.ell(12, cy, 4.2 + stage * 1.3, ry, P.body);
    c.ell(12, cy + 1.6, 2.4 + stage * 0.6, 2.0 + stage * 0.4, P.belly);
    // tall pointed ears with dark tips
    c.mtri(6, 11, 5.2, 3.5 - stage * 0.5, 10, 8.5, P.body);
    c.mtri(6.6, 8.6, 5.6, 4.8 - stage * 0.5, 8.2, 7.4, P.accent2);
    c.m(8, 11, mul(P.body, 0.9));
    // lightning cheek marks
    c.m(6, 15, bolt); c.m(5, 16, bolt); c.m(6, 17, bolt);
    eyeY = static_cast<int>(cy) - 1; eyeA = 8;
}

// 2: Ember canine - flame crest and tail (Wound, Focus, burst)
void paintFlame(Cv& c, int stage, int f, const Pal& P, int& eyeY, int& eyeA) {
    const u32 hot = P.accent, deep = P.accent2;
    // tail flame
    c.tri(18, 19, 22, 12 - stage - f, 21, 19, deep);
    c.tri(19, 19, 21.5, 14 - stage - f, 20.5, 19, hot);
    const double ry = 4.0 + stage * 1.0, cy = 16.4 - (stage - 1) * 0.6;
    feet(c, 20, mul(P.body, 0.75), f, 3 + stage);
    c.ell(12, cy, 4.2 + stage * 1.3, ry, P.body);
    c.ell(12, cy + 1.6, 2.4 + stage * 0.6, 2.0 + stage * 0.4, P.belly);
    // small ears
    c.mtri(6.5, 10.5, 6, 6.5, 9.5, 8.5, P.body);
    // flame crest (grows with stage)
    const int fl = f ? 1 : 0;
    c.tri(10, 9, 12, 2 - stage - fl, 14, 9, deep);
    c.tri(10.8, 9, 12, 4.2 - stage - fl, 13.2, 9, hot);
    if (stage >= 2) { c.tri(7.5, 9.5, 8.5, 3.5 - fl, 10.5, 9, deep); c.tri(13.5, 9.5, 15.5, 3.5 - fl, 16.5, 9.5, deep); c.tri(8.2, 9.5, 8.8, 5.5 - fl, 9.8, 9.5, hot); }
    if (stage == 3) { c.m(5, 13, hot); c.m(4, 15, deep); c.m(5, 17, hot); }  // embers on the shoulders
    eyeY = static_cast<int>(cy) - 1; eyeA = 8;
}

// 3: Wall of rock - cracked stone body, moss (Armor, Counter)
void paintRock(Cv& c, int stage, int f, const Pal& P, int& eyeY, int& eyeA) {
    const double rx = 6 + stage * 1.2, ry = 5.4 + stage * 1.0, cy = 15.5 - (stage - 1) * 0.5;
    feet(c, 20, mul(P.body, 0.7), f, 5);
    c.ell(12, cy, rx, ry, P.body);
    // jagged top plates
    for (int i = 0; i < 2 + stage; ++i) c.mtri(12 - i * 2.4 - 0.5, cy - ry + 1.2, 12 - i * 2.4 - 2, cy - ry - 1.3 - (i % 2), 12 - i * 2.4 + 1.4, cy - ry + 0.6, mul(P.body, 1.0 - 0.06 * i));
    // cracks and chips
    const u32 crack = mul(P.body, 0.5);
    c.line(8, static_cast<int>(cy) - 2, 10, static_cast<int>(cy) + 1, crack); c.line(10, static_cast<int>(cy) + 1, 9, static_cast<int>(cy) + 4, crack);
    c.line(15, static_cast<int>(cy) - 3, 14, static_cast<int>(cy), crack);
    c.dot(15.5, cy + 3.5, 1.4, P.accent2);   // lichen patch
    c.dot(8, cy + 4, 1.1, P.accent2);
    c.ell(12, cy + 2.3, 3 + stage * 0.5, 2, P.belly);
    eyeY = static_cast<int>(cy) - 2; eyeA = 8;
}

// 4: Healer droplet - teardrop body, bubbles, fins (Heal, Regen, Cleanse)
void paintDrop(Cv& c, int stage, int f, const Pal& P, int& eyeY, int& eyeA) {
    const double cy = 15.2, rx = 5.8 + stage * 0.7;
    c.ell(12, cy, rx, 5.4 + stage * 0.6, P.body);
    c.tri(12 - 3.6, cy - 2, 12, 4.5 - stage - f, 12 + 3.6, cy - 2, mul(P.body, 1.1));
    c.ell(12, cy + 2, 3.2, 2.4, P.belly);
    c.dot(9, cy - 2, 1.2, rgb(240, 250, 255));                         // shine
    if (stage >= 2) { c.mtri(5.5, cy, 1.5, cy - 3 + f, 5.5, cy + 3, P.accent); c.m(3, cy + 1, P.belly); }  // fins
    c.dot(19.5, 6 + f, 1.2, P.belly); c.dot(4.5, 8 - f, 1.0, P.belly); c.dot(20, 11, 0.9, P.accent);  // bubbles
    eyeY = static_cast<int>(cy) - 1; eyeA = 8;
}

// 5: Curse moth - big patterned wings, antennae (Mark, Doom, Seal)
void paintMoth(Cv& c, int stage, int f, const Pal& P, int& eyeY, int& eyeA) {
    const int fl = f ? 1 : 0;
    const double ws = 5 + stage * 1.0;
    c.ell(6.2, 12 - fl, ws * 0.95, ws + 0.5, P.accent2);       // upper wings
    c.ell(7.4, 17.5 + fl * 0.5, ws * 0.7, ws * 0.7, mul(P.accent2, 0.85));
    c.ell(17.8, 12 - fl, ws * 0.95, ws + 0.5, P.accent2);
    c.ell(16.6, 17.5 + fl * 0.5, ws * 0.7, ws * 0.7, mul(P.accent2, 0.85));
    c.dot(6, 11 - fl, 1.9 + stage * 0.3, P.accent); c.dot(18, 11 - fl, 1.9 + stage * 0.3, P.accent);   // moon spots
    c.dot(6.5, 11 - fl, 0.9, mul(P.accent2, 0.8)); c.dot(17.5, 11 - fl, 0.9, mul(P.accent2, 0.8));
    c.ell(12, 14.5, 3.4, 5.4, P.body);                                        // fuzzy body
    c.ell(12, 15.5, 2.2, 3.4, P.belly);
    c.mline(10, 9, 7, 4 - stage / 2, P.accent); c.m(7, 3 - stage / 2, P.accent); c.m(6, 4 - stage / 2, P.accent);   // antennae
    eyeY = 11; eyeA = 9;
}

// 6: Hit-and-run bird - spread wings, beak, tail feathers (Pivot, Priority, Haste)
void paintBird(Cv& c, int stage, int f, const Pal& P, int& eyeY, int& eyeA) {
    const int up = f ? -2 : 1;
    const double ws = 4 + stage * 1.2;
    c.mtri(8, 14, 8 - ws - 2, 12 + up - stage, 8 - ws + 1, 17 + up, P.accent2);        // wings
    c.mtri(8, 14, 8 - ws - 2, 12 + up - stage, 8 - ws * 0.4, 9 + up, mul(P.accent2, 1.1));
    if (stage >= 2) c.mtri(8, 16, 4 - stage, 18 + up, 7, 19, mul(P.accent2, 0.8));
    c.ell(12, 15.5, 5.2 + stage * 0.5, 5 + stage * 0.5, P.body);
    c.ell(12, 17, 3, 3, P.belly);
    c.mtri(10, 20, 8.5, 23, 11.5, 21, P.accent2); c.rect(11, 20, 12, 23, P.accent2);     // tail feathers
    c.tri(10.5, 13.5, 12, 17.5, 13.5, 13.5, P.accent);                                    // beak
    if (stage >= 2) { c.tri(10, 9, 12, 4 - stage, 14, 9, P.accent2); }                    // crest
    eyeY = 11 - (stage > 1 ? 0 : 0); eyeA = 8;
}

// 7: Support crystal - faceted diamond, sparkles (Shield, Focus, Daze)
void paintCrystal(Cv& c, int stage, int f, const Pal& P, int& eyeY, int& eyeA) {
    const double rx = 5.2 + stage * 0.9, ry = 7 + stage * 0.9, cy = 13.5;
    for (int y = 0; y < N; ++y)
        for (int x = 0; x < N; ++x) {
            const double dx = std::fabs(x + 0.5 - 12) / rx, dy = std::fabs(y + 0.5 - cy) / ry;
            if (dx + dy > 1.0) continue;
            double fcs = (x + 0.5 < 12) ? 1.08 : 0.88;       // two-tone facets
            if (y + 0.5 < cy - ry * 0.2) fcs += 0.08;
            c.set(x, y, mul(P.body, fcs));
        }
    c.line(12, static_cast<int>(cy - ry) + 1, 12, static_cast<int>(cy + ry) - 1, mul(P.body, 1.2));
    c.mtri(7, 8, 5, 2 - stage, 9, 6, P.accent);                                       // pink shards
    if (stage >= 2) c.tri(11, 6, 12, 1, 13, 6, P.accent);
    const int sp = f ? 1 : 0;
    c.m(3 + sp, 6, rgb(255, 255, 255)); c.m(3 + sp, 5, rgb(255, 255, 255)); c.m(2 + sp, 6, rgb(255, 255, 255)); c.m(4 + sp, 6, rgb(255, 255, 255));  // sparkles
    c.ell(12, cy + 3, 2.4, 2, P.belly, false);
    eyeY = 12; eyeA = 9;
}

// 8: Trapper mole - big claws, snout, miner's helmet (Trap, Dig, Wound)
void paintMole(Cv& c, int stage, int f, const Pal& P, int& eyeY, int& eyeA) {
    c.ell(12, 22.3, 9, 2, mul(P.body, 0.65), false);                                  // dirt mound
    const double rx = 5.8 + stage * 0.8, ry = 4.8 + stage * 0.7, cy = 15.5;
    c.ell(12, cy, rx, ry, P.body);
    c.ell(12, cy + 2, 3.2, 2.2, P.belly);
    c.ell(12, cy - 0.5, 1.8, 1.4, rgb(235, 150, 170), false);                         // snout
    // claws
    const int dig = f ? 1 : 0;
    c.mrect(2, 15 + dig, 4, 19 + dig, mul(P.body, 0.9));
    c.mrect(1, 19 + dig, 1, 20 + dig, P.accent); c.mrect(3, 20 + dig, 3, 21 + dig, P.accent); c.mrect(5, 19 + dig, 5, 20 + dig, P.accent);
    if (stage >= 2) { c.ell(12, 9.5, 5.5, 3, rgb(210, 190, 90)); c.rect(8, 11, 15, 11, rgb(170, 150, 60)); c.dot(12, 8, 1.1, rgb(255, 255, 190)); }  // helmet + lamp
    eyeY = static_cast<int>(cy) - 3; eyeA = 8;
}

// 9: Control wisp - floating teardrop, ice spikes, snowflakes (Slow, Stun, Armor)
void paintFrost(Cv& c, int stage, int f, const Pal& P, int& eyeY, int& eyeA) {
    const double cy = 12.5 + (f ? 0.5 : 0);
    const double rx = 5 + stage * 0.9;
    c.ell(12, cy, rx, 5.6 + stage * 0.7, P.body);
    c.tri(12 - rx + 1, cy + 3, 12, 22 - f, 12 + rx - 1, cy + 3, mul(P.body, 0.95));      // trailing wisp
    c.ell(12, cy + 1.5, 3, 2.6, P.belly);
    for (int i = 0; i < 2 + stage; ++i) c.mtri(12 - 5 + i * 2, cy - 4, 12 - 6 + i * 2.4, cy - 9 - (i % 2) - stage * 0.6, 12 - 3 + i * 2.2, cy - 4.5, P.accent);   // ice crown
    if (stage >= 2) { c.mtri(6, cy + 1, 1.5, cy - 1, 6, cy + 4, P.accent); }                 // shoulder shards
    c.m(3, 6, rgb(255, 255, 255)); c.m(3, 5, rgb(220, 240, 255)); c.m(2, 6, rgb(220, 240, 255)); c.m(4, 6, rgb(220, 240, 255)); c.m(3, 7, rgb(220, 240, 255));   // snowflake
    eyeY = static_cast<int>(cy) - 1; eyeA = 8;
}

// 10: Trickster rat - round ears, mirror disc on belly, floating shards (Mimic, Reflect)
void paintMirror(Cv& c, int stage, int f, const Pal& P, int& eyeY, int& eyeA) {
    const double cy = 15.5, ry = 4.8 + stage * 0.8;
    // thin tail
    c.line(18, 20, 21, 18, mul(P.body, 0.7)); c.line(21, 18, 22 - f, 14, mul(P.body, 0.7));
    feet(c, 20, mul(P.body, 0.75), f, 4);
    c.ell(12, cy, 5.2 + stage * 0.7, ry, P.body);
    c.dot(7, 9.5, 2.8 + stage * 0.3, P.body); c.dot(17, 9.5, 2.8 + stage * 0.3, P.body);
    c.dot(7, 9.7, 1.5, rgb(240, 170, 190), false); c.dot(17, 9.7, 1.5, rgb(240, 170, 190), false);   // inner ears
    // mirror disc
    c.ell(12, cy + 2.2, 3.4, 3.4, P.accent2, false);
    c.ell(12, cy + 2.2, 2.7, 2.7, rgb(225, 238, 255), false);
    c.line(10, static_cast<int>(cy) + 3, 12, static_cast<int>(cy) + 1, rgb(255, 255, 255));
    if (stage >= 2) { c.mtri(3, 8, 1, 5 + f, 4.5, 6.2, rgb(225, 238, 255)); c.mtri(2, 13, 0.5, 11, 3.5, 11.2, rgb(200, 215, 245)); }  // floating shards
    eyeY = static_cast<int>(cy) - 2; eyeA = 8;
}

// 11: Time owl - tufted ears, clock belly (cooldown tricks, Doom)
void paintOwl(Cv& c, int, int f, const Pal& P, int& eyeY, int& eyeA) {
    feet(c, 21, P.accent, f, 3);
    c.ell(12, 14, 6.6, 7.2, P.body);
    c.mtri(7, 8, 5, 2.5, 10, 6.5, mul(P.body, 0.9));                                  // ear tufts
    c.ell(12, 16.5, 4.4, 4.4, P.accent2, false);                                      // clock rim
    c.ell(12, 16.5, 3.6, 3.6, P.belly, false);
    const int hand = f ? 1 : 0;
    c.line(12, 16, 12, 14, rgb(40, 30, 20)); c.line(12, 16, 14 + hand, 16, rgb(40, 30, 20)); c.set(12, 19, rgb(120, 90, 40)); c.set(9, 16, rgb(120, 90, 40)); c.set(15, 16, rgb(120, 90, 40));
    c.dot(8.5, 10.8, 2.4, P.accent2, false); c.dot(15.5, 10.8, 2.4, P.accent2, false);   // big eye rings
    c.mtri(4.5, 13, 2, 17, 5.5, 18, mul(P.body, 0.8));                                // wings
    eyeY = 10; eyeA = 7;
}

// 12: Armoured bug - plate bands, mandibles, rivets (Armor stacking, Rust)
void paintArmor(Cv& c, int, int f, const Pal& P, int& eyeY, int& eyeA) {
    for (int i = 0; i < 3; ++i) {      // legs
        const int ly = 15 + i * 2;
        c.mline(6, ly, 2, ly + 2 + (f && i == 1 ? 1 : 0), mul(P.body, 0.6));
    }
    c.ell(12, 15.5, 7, 5.6, mul(P.body, 0.9));
    for (int i = 0; i < 3; ++i) {      // plate bands
        const int y = 11 + i * 3;
        c.rect(6 + (i == 0 ? 1 : 0), y, 17 - (i == 0 ? 1 : 0), y + 1, P.belly);
        c.rect(6, y + 2, 17, y + 2, mul(P.body, 0.5));
        c.m(6 + i, y, P.accent2); c.m(7 + i, y + 1, P.accent2);
    }
    c.mtri(8, 12, 6.5, 16.5, 10, 14.5, mul(P.belly, 0.9));                            // mandibles
    c.mline(9, 9, 6, 5 - f, mul(P.body, 0.6));                                        // antennae
    eyeY = 9; eyeA = 8;
}

// 13: Vampire bat - big membrane wings, fangs (Drain, Wound, Blood Pact)
void paintBat(Cv& c, int, int f, const Pal& P, int& eyeY, int& eyeA) {
    const int fl = f ? 2 : 0;
    c.mtri(8, 12, 0, 6 + fl, 2, 13, P.accent2);
    c.mtri(8, 14, 0, 16 + fl, 3, 11, mul(P.accent2, 0.9));
    c.mtri(8, 15, 2, 21 - fl, 6, 17, mul(P.accent2, 0.85));
    c.mline(8, 12, 1, 7 + fl, mul(P.body, 0.7)); c.mline(8, 13, 1, 15 + fl, mul(P.body, 0.7));
    c.ell(12, 15, 4.8, 5.4, P.body);
    c.ell(12, 16.5, 3, 3.2, P.belly);
    c.mtri(8, 11, 7.5, 3, 11, 9, P.body);                                              // tall ears
    c.set(10, 17, rgb(255, 255, 255)); c.set(13, 17, rgb(255, 255, 255)); c.set(10, 18, rgb(255, 255, 255)); c.set(13, 18, rgb(255, 255, 255));  // fangs
    eyeY = 12; eyeA = 9;
}

// 14: Mimic jelly cube - isometric block that copies things
void paintCube(Cv& c, int, int f, const Pal& P, int& eyeY, int& eyeA) {
    const int jig = f ? 1 : 0;
    c.rect(5, 10 + jig, 18, 21, P.body);
    c.rect(5, 6 + jig, 18, 9 + jig, mul(P.body, 1.2));                                // lighter top face
    c.rect(5, 17, 18, 21, mul(P.body, 0.85));
    c.rect(4, 11 + jig, 4, 20, mul(P.body, 0.9)); c.rect(19, 11 + jig, 19, 20, mul(P.body, 0.9));
    c.rect(6, 7 + jig, 8, 8 + jig, rgb(255, 255, 255)); c.set(7, 7 + jig, rgb(255, 255, 255));   // gloss
    c.rect(8, 15, 15, 19, P.belly);
    c.set(17, 12 + jig, P.accent); c.set(6, 18, P.accent);                           // absorbed bits
    eyeY = 12 + jig; eyeA = 8;
}

// 15: Phoenix - wings of flame, flaming crest and tail (Rebirth, Shield, Solar Dive)
void paintPhoenix(Cv& c, int, int f, const Pal& P, int& eyeY, int& eyeA) {
    const int fl = f ? 1 : 0;
    c.mtri(8, 13, 0, 6 - fl, 2, 15, P.accent2); c.mtri(8, 14, 1.5, 9 - fl, 3, 15.5, P.accent);   // flame wings
    c.mtri(8, 16, 1, 18 + fl, 4, 14, mul(P.accent2, 0.9));
    c.tri(9, 19, 12, 24, 15, 19, P.accent2); c.tri(10, 19, 12, 22.5 + fl * 0.5, 14, 19, P.accent);    // tail
    c.ell(12, 14.5, 5, 5.4, P.body);
    c.ell(12, 16, 3, 3.4, P.belly);
    c.tri(9, 9.5, 12, 2 - fl, 15, 9.5, P.accent2); c.tri(10.5, 9.5, 12, 4.5 - fl, 13.5, 9.5, P.accent);   // crest
    c.tri(10.5, 12.5, 12, 15.5, 13.5, 12.5, rgb(255, 200, 70));                        // beak
    eyeY = 10; eyeA = 8;
}

// 16: Chaos prism - rainbow-striped body, extra eyes, shards (random effects)
void paintChaos(Cv& c, int, int f, const Pal&, int& eyeY, int& eyeA) {
    const double cy = 14.5;
    for (int y = 0; y < N; ++y)
        for (int x = 0; x < N; ++x) {
            const double dx = (x + 0.5 - 12) / 7, dy = (y + 0.5 - cy) / 7;
            if (dx * dx + dy * dy > 1.0) continue;
            const double h = std::fmod((y + (f ? 2 : 0)) * 28.0, 360.0);
            const double t = h / 60.0;
            const int seg = static_cast<int>(t) % 6;
            const double fr = t - std::floor(t);
            int r = 0, g = 0, b = 0;
            switch (seg) { case 0: r = 255; g = static_cast<int>(255 * fr); b = 60; break; case 1: r = static_cast<int>(255 * (1 - fr)); g = 255; b = 60; break; case 2: r = 60; g = 255; b = static_cast<int>(255 * fr); break;
                           case 3: r = 60; g = static_cast<int>(255 * (1 - fr)); b = 255; break; case 4: r = static_cast<int>(255 * fr); g = 60; b = 255; break; default: r = 255; g = 60; b = static_cast<int>(255 * (1 - fr)); break; }
            c.set(x, y, mul(rgb(r, g, b), 0.85 + 0.2 * (1 - (y - cy + 7) / 14.0)));
        }
    c.mtri(6, 9, 3, 3, 9, 7, rgb(255, 255, 255)); c.mtri(5, 14, 0.5, 13, 5, 17, rgb(210, 230, 255));   // prism shards
    c.tri(10, 7.5, 12, 1.5, 14, 7.5, rgb(255, 255, 255));
    c.set(12, 9 + f, rgb(30, 20, 50)); c.set(12, 10 + f, rgb(250, 250, 250)); c.set(12, 17, rgb(30, 20, 50));   // extra eyes
    eyeY = 12; eyeA = 8;
}

// 17: Fungus - spotted mushroom cap on a stalk body (Wound clouds, Regen)
void paintShroom(Cv& c, int stage, int f, const Pal& P, int& eyeY, int& eyeA) {
    const double cy = 16.5, ry = 4.8 + stage * 0.6;
    feet(c, 21, mul(P.body, 0.8), f, 3);
    c.ell(12, cy, 4.2 + stage * 0.6, ry, P.body);
    const double capR = 7 + stage * 1.2;
    c.ell(12, 10, capR, 4.6 + stage * 0.6, P.accent);
    c.rect(static_cast<int>(12 - capR), 11, static_cast<int>(12 + capR) - 1, 11, mul(P.accent, 0.7));
    c.dot(8.5, 8.5, 1.3, rgb(250, 240, 245), false); c.dot(14.5, 7.5, 1.6, rgb(250, 240, 245), false); c.dot(12, 10.6, 1.0, rgb(250, 240, 245), false);
    if (stage >= 2) { c.dot(18.5, 10, 1.1, rgb(250, 240, 245), false); c.dot(5, 10.5, 1.1, rgb(250, 240, 245), false); c.ell(5.5, 4.8, 1.8, 1.4, P.accent, false); c.rect(5, 6, 5, 7, P.body); }  // baby mushroom
    c.m(3, 14 - f * 2, mul(P.accent, 1.2)); c.m(2, 17 - f, mul(P.accent, 1.2));       // spores
    eyeY = static_cast<int>(cy) - 1; eyeA = 9;
}

// 18: Desert - sand pup, becoming a coiled dune serpent (Daze, Slow, Evade, Trap)
void paintSand(Cv& c, int stage, int f, const Pal& P, int& eyeY, int& eyeA) {
    if (stage == 1) {
        c.line(18, 18, 21, 16, P.accent); c.line(21, 16, 21, 13 - f, P.accent);
        feet(c, 20, mul(P.body, 0.75), f, 4);
        c.ell(12, 15.5, 5.4, 4.8, P.body);
        c.ell(12, 17, 3, 2.4, P.belly);
        c.ell(5.8, 12.8, 2, 3.2, P.accent, false); c.ell(18.2, 12.8, 2, 3.2, P.accent, false);   // floppy ears
        c.m(2, 21, mul(P.body, 0.8)); c.m(3, 22, mul(P.body, 0.8));                   // dust
        eyeY = 13; eyeA = 8;
    } else {
        c.ell(12, 20, 8.5, 2.6, mul(P.body, 0.85));                                   // coils
        c.ell(12, 16.5, 6.5, 3.4, P.body);
        c.ell(12, 17.5, 4.2, 1.8, P.belly, false);
        c.ell(12, 10.5, 4.6, 4.8, P.body);                                            // raised head
        c.mtri(8, 8, 5, 3.5 - f, 9.5, 6.5, P.accent); c.mtri(12 - 0.5, 6, 12, 2, 12.5, 6, P.accent);   // crest fins
        for (int i = 0; i < 3; ++i) c.mrect(5 - i, 15 + i, 6 - i, 15 + i, mul(P.accent, 0.9));
        c.dot(4, 6, 1.0, P.belly, false); c.dot(20, 5, 1.2, P.belly, false);          // swirling sand
        eyeY = 9; eyeA = 9;
    }
}

// 19: Clockwork dog - gear on its flank, copper plates, bolts (Empower, Priority)
void paintCog(Cv& c, int stage, int f, const Pal& P, int& eyeY, int& eyeA) {
    const double ry = 4.7 + stage * 0.7, cy = 15.7;
    feet(c, 20, mul(P.accent2, 0.9), f, 4);
    c.line(18, 18, 21, 16, mul(P.accent2, 0.8)); c.dot(21.5, 15, 1.4, P.accent, false);
    c.ell(12, cy, 5.2 + stage * 0.7, ry, P.body);
    c.ell(12, cy + 1.7, 2.8 + stage * 0.4, 2.2, P.belly);
    c.mtri(6.5, 10.5, 6, 5.5, 10, 8.5, P.accent2); c.mtri(6.5, 10.5, 6.8, 7, 8.6, 9.6, mul(P.accent2, 1.2));   // metal ears
    // gear on flank
    const double gr = 2.7 + stage * 0.4;
    c.ell(17.8, cy + 0.5, gr, gr, P.accent, false);
    for (int i = 0; i < 8; ++i) { const double a = i * 0.785 + (f ? 0.39 : 0); c.set(static_cast<int>(17.8 + std::cos(a) * (gr + 1)), static_cast<int>(cy + 0.5 + std::sin(a) * (gr + 1)), P.accent); }
    c.dot(17.8, cy + 0.5, 1.0, mul(P.accent2, 0.6), false);
    c.m(7, 14, rgb(220, 220, 230)); c.m(6, 17, rgb(220, 220, 230));                   // bolts
    if (stage >= 2) { c.tri(10, 9, 12, 3, 14, 9, P.accent); c.dot(12, 5.5, 1.1, mul(P.accent2, 0.6), false); }   // gear crown
    eyeY = 13; eyeA = 8;
}

// 20: Shell - open clam with a pearl, growing armour spikes (Armor, Thorns, support)
void paintClam(Cv& c, int stage, int f, const Pal& P, int& eyeY, int& eyeA) {
    const double sr = 7 + stage * 1.0;
    const int open = f ? 1 : 0;
    c.ell(12, 17.5, sr, 4, P.accent2);                                                // lower shell
    c.ell(12, 11 - open, sr, 5.2 + stage * 0.5, P.body);                              // upper shell
    for (int i = -2; i <= 2; ++i) c.line(12 + i * 2, 8 - open, 12 + i * 3, 13 - open, mul(P.body, 0.75));   // ridges
    c.ell(12, 14.5, 3.8, 2.4, rgb(150, 110, 140), false);                             // soft body
    c.dot(12, 16, 2.2 + stage * 0.2, P.belly, false); c.set(11, 15, rgb(255, 255, 255));   // pearl
    if (stage >= 2) { for (int i = 0; i < 3; ++i) c.mtri(8 + i * 1.5, 7.5 - open, 6.5 + i * 1.7, 3.5 - open, 10 + i * 1.5, 7, P.belly); }
    eyeY = 13; eyeA = 8;
}

// 21: Mountain goat - curled horns, beard, rocky shoulders (rams, Trap, Counter)
void paintGoat(Cv& c, int stage, int f, const Pal& P, int& eyeY, int& eyeA) {
    feet(c, 20, rgb(80, 75, 85), f, 4);
    c.ell(12, 15.5, 5.4 + stage * 0.8, 4.9 + stage * 0.7, P.body);
    c.ell(12, 17, 3, 2.6, P.belly);
    // curled horns
    const double hs = 1 + stage * 0.35;
    c.mline(8, 9, 5, 6, P.accent2); c.mline(5, 6, 3, static_cast<int>(8 - hs), P.accent2); c.mline(3, static_cast<int>(8 - hs), 4, static_cast<int>(11 - hs), P.accent2);
    c.mline(7, 10, 4, 7, P.accent2);
    c.ell(12, 18.5, 1.4, 2.2, P.belly, false);                                         // beard
    if (stage >= 2) { c.mtri(6, 13, 3, 11, 5, 17, rgb(120, 118, 130)); c.tri(9, 9, 12, 5, 15, 9, rgb(245, 250, 255)); }   // rock shoulders + snow cap
    c.m(10, 14 + f, mul(P.body, 0.8));
    eyeY = 12; eyeA = 8;
}

// 22: Sea line - round fish, flat ray, then a spiny leviathan (tempo, Trap, Maelstrom)
void paintFish(Cv& c, int stage, int f, const Pal& P, int& eyeY, int& eyeA) {
    const int fl = f ? 1 : 0;
    if (stage == 1) {
        c.tri(9, 18, 12, 23, 15, 18, P.accent);                                       // tail fin
        c.ell(12, 14, 6.2, 5.4, P.body);
        c.ell(12, 16, 4, 3, P.belly);
        c.mtri(6, 14, 1.5, 12 - fl, 5.5, 17.5, P.accent);                              // side fins
        c.tri(10, 9, 12, 4 - fl, 14, 9, P.accent);                                      // dorsal fin
        eyeY = 12; eyeA = 8;
    } else if (stage == 2) {
        c.mtri(12, 14, 0, 11 - fl, 5, 17.5, P.accent);                                  // wing fins
        c.mtri(12, 13, 0.5, 15 + fl, 4, 18, mul(P.accent, 0.85));
        c.ell(12, 14.5, 6, 5, P.body); c.ell(12, 16.5, 3.6, 2.6, P.belly);
        c.line(12, 19, 12, 23, P.accent2); c.line(11, 22, 13, 22, P.accent2);           // stinger tail
        eyeY = 12; eyeA = 8;
    } else {
        c.mtri(12, 14, 0, 9 - fl, 4, 18, P.accent);
        c.tri(8, 20, 12, 24, 16, 20, P.accent);
        c.ell(12, 14, 7.5, 6.8, P.body); c.ell(12, 17, 5, 3.6, P.belly);
        for (int i = 0; i < 4; ++i) c.tri(8 + i * 2.7, 8, 9.2 + i * 2.7, 3 + (i % 2) - fl, 10.5 + i * 2.7, 8, P.accent2);   // spiny crest
        c.set(9, 18, rgb(255, 255, 255)); c.set(14, 18, rgb(255, 255, 255)); c.set(9, 19, rgb(255, 255, 255)); c.set(14, 19, rgb(255, 255, 255));   // fangs
        eyeY = 11; eyeA = 7;
    }
}

// 23: Eclipse - crescent behind a dark body, glowing eyes (Mark, Doom, Evade)
void paintEclipse(Cv& c, int, int f, const Pal& P, int& eyeY, int& eyeA) {
    c.ell(12, 11, 9, 9, P.accent, false);                                              // moon disc
    c.ell(14.5, 10, 8, 8.3, rgb(0, 0, 0), false);
    for (int y = 0; y < N; ++y) for (int x = 0; x < N; ++x) if (c.get(x, y) == rgb(0, 0, 0)) c.p[static_cast<size_t>(y * N + x)] = 0;   // carve crescent
    c.ell(12, 15.5, 6.2, 5.6, P.body);
    c.ell(12, 17, 3.8, 3, mul(P.body, 1.25));
    c.mtri(7, 11, 5, 5.5, 9.5, 9, P.body);                                             // pointed ears
    c.set(19, 5 + f, rgb(255, 255, 255)); c.set(21, 9, rgb(255, 255, 255)); c.set(3, 18 - f, rgb(255, 255, 255)); c.set(20, 4 + f, rgb(220, 220, 255));  // stars
    eyeY = 13; eyeA = 8;
}

// 24: Garden turtle - mossy dome shell with flowers (Regen, Shield, Root snare)
void paintTurtle(Cv& c, int, int f, const Pal& P, int& eyeY, int& eyeA) {
    feet(c, 20, P.belly, f, 6);
    c.ell(12, 14, 8.5, 7, P.body);                                                    // shell dome
    c.rect(4, 14, 19, 15, mul(P.body, 0.7));
    for (int i = 0; i < 3; ++i) c.line(8 + i * 4, 8, 8 + i * 4, 14, mul(P.body, 0.75));
    c.ell(9, 9.5, 3, 1.6, P.accent2, false); c.ell(15.5, 11, 2.6, 1.4, P.accent2, false);   // moss
    c.m(7, 7 - f, rgb(255, 150, 175)); c.m(7, 8 - f, rgb(255, 230, 120)); c.m(5, 10, rgb(255, 255, 255));   // flowers
    c.ell(12, 18, 4.4, 3.2, P.belly);                                                  // head
    eyeY = 17; eyeA = 9;
}

// 25: Ice bird - spiked wings with icicles, frozen crest (Spread slow, Dive)
void paintIceBird(Cv& c, int, int f, const Pal& P, int& eyeY, int& eyeA) {
    const int up = f ? -2 : 1;
    c.mtri(8, 13, -1, 8 + up, 3, 16 + up, P.accent);
    c.mtri(8, 14, 0, 14 + up, 3, 19 + up, mul(P.accent, 0.85));
    for (int i = 0; i < 3; ++i) c.mtri(2 + i * 1.8, 16 + up + i, 1 + i * 2, 20 + up + i, 3 + i * 1.8, 17 + up + i, rgb(235, 248, 255));   // icicle tips
    c.ell(12, 15, 5, 5.2, P.body);
    c.ell(12, 16.5, 3, 3, rgb(245, 250, 255));
    c.tri(10.5, 13, 12, 17, 13.5, 13, rgb(250, 190, 90));
    for (int i = 0; i < 3; ++i) c.tri(9 + i * 2, 9.5, 10 + i * 2, 3 - (i == 1 ? 1 : 0), 11 + i * 2, 9.5, rgb(210, 240, 255));  // ice crest
    c.m(5, 22, rgb(255, 255, 255));
    eyeY = 11; eyeA = 8;
}

// 26: Lava beast - dark rock with glowing cracks and horns (Wound, Armor, Thorns)
void paintLava(Cv& c, int, int f, const Pal& P, int& eyeY, int& eyeA) {
    feet(c, 20, P.body, f, 6);
    c.ell(12, 14.5, 8.2, 7, P.body);
    c.mtri(6.5, 9, 4, 2.5, 9.5, 7, mul(P.body, 1.2)); c.mtri(7, 8, 6, 4.5, 8.7, 7, P.accent);    // horns with hot tips
    const u32 lava = P.accent;
    const int gl = f ? 1 : 0;
    c.line(8, 12, 10, 15, lava); c.line(10, 15, 9, 19, lava); c.line(16, 11, 14, 14, lava); c.line(14, 14, 15, 18 + gl, lava); c.line(11, 17, 13, 18, mix(lava, rgb(255, 240, 150), 0.6));
    c.set(12, 12, mix(lava, rgb(255, 240, 150), 0.5));
    c.mtri(5, 14, 2, 12, 4, 18, mul(P.body, 0.85));
    eyeY = 10; eyeA = 8;
}

struct LineInfo {
    Pal pal;
    void (*paint)(Cv&, int, int, const Pal&, int&, int&);
    int eyeStyle;  // 0 friendly, 2 fierce, 3 glowing
};

const LineInfo& lineInfo(int line) {
    static const LineInfo table[27] = {
        /* 0  Mossling   */ {{rgb(112, 160, 86), rgb(196, 224, 150), rgb(70, 190, 100), rgb(130, 92, 56)}, paintMoss, 0},
        /* 1  Zapling    */ {{rgb(242, 204, 62), rgb(255, 240, 175), rgb(255, 255, 120), rgb(80, 55, 20)}, paintFox, 0},
        /* 2  Cindrel    */ {{rgb(226, 98, 62), rgb(255, 208, 160), rgb(255, 205, 60), rgb(230, 80, 30)}, paintFlame, 0},
        /* 3  Pebblet    */ {{rgb(142, 142, 152), rgb(196, 190, 178), rgb(80, 80, 90), rgb(110, 160, 90)}, paintRock, 0},
        /* 4  Dewdrip    */ {{rgb(84, 164, 232), rgb(176, 224, 250), rgb(60, 130, 210), rgb(255, 255, 255)}, paintDrop, 0},
        /* 5  Gloomoth   */ {{rgb(150, 110, 200), rgb(212, 184, 236), rgb(250, 232, 130), rgb(96, 62, 150)}, paintMoth, 0},
        /* 6  Gustling   */ {{rgb(128, 194, 242), rgb(246, 246, 252), rgb(250, 172, 52), rgb(90, 150, 220)}, paintBird, 0},
        /* 7  Glimmerling*/ {{rgb(118, 228, 232), rgb(200, 248, 250), rgb(255, 150, 222), rgb(255, 255, 255)}, paintCrystal, 0},
        /* 8  Burrowby   */ {{rgb(154, 108, 72), rgb(214, 174, 134), rgb(238, 228, 204), rgb(90, 60, 40)}, paintMole, 0},
        /* 9  Frostwisp  */ {{rgb(176, 228, 250), rgb(242, 250, 255), rgb(112, 172, 232), rgb(255, 255, 255)}, paintFrost, 0},
        /* 10 Mirrorat   */ {{rgb(172, 168, 204), rgb(232, 242, 255), rgb(255, 255, 255), rgb(120, 120, 160)}, paintMirror, 0},
        /* 11 Chronowl   */ {{rgb(64, 84, 146), rgb(236, 220, 176), rgb(242, 192, 62), rgb(190, 140, 40)}, paintOwl, 0},
        /* 12 Ironmite   */ {{rgb(122, 132, 148), rgb(168, 178, 194), rgb(232, 142, 52), rgb(232, 142, 52)}, paintArmor, 2},
        /* 13 Sangrebat  */ {{rgb(112, 44, 74), rgb(176, 96, 116), rgb(230, 230, 235), rgb(160, 44, 64)}, paintBat, 2},
        /* 14 Mimicube   */ {{rgb(72, 204, 174), rgb(168, 244, 226), rgb(255, 255, 255), rgb(255, 160, 80)}, paintCube, 0},
        /* 15 Ashenfeather*/ {{rgb(246, 164, 56), rgb(255, 224, 128), rgb(255, 214, 70), rgb(232, 72, 40)}, paintPhoenix, 0},
        /* 16 Kaleidra   */ {{rgb(200, 200, 220), rgb(240, 240, 250), rgb(255, 255, 255), rgb(255, 255, 255)}, paintChaos, 0},
        /* 17 Sporeling  */ {{rgb(228, 208, 174), rgb(246, 236, 214), rgb(174, 84, 152), rgb(174, 84, 152)}, paintShroom, 0},
        /* 18 Sandpup    */ {{rgb(228, 194, 114), rgb(248, 228, 176), rgb(204, 152, 74), rgb(204, 152, 74)}, paintSand, 0},
        /* 19 Cogpup     */ {{rgb(204, 134, 84), rgb(236, 190, 140), rgb(232, 204, 84), rgb(150, 160, 172)}, paintCog, 0},
        /* 20 Pearlclam  */ {{rgb(238, 172, 190), rgb(252, 252, 255), rgb(236, 226, 248), rgb(150, 100, 150)}, paintClam, 0},
        /* 21 Cragoat    */ {{rgb(194, 194, 204), rgb(240, 240, 246), rgb(222, 202, 154), rgb(222, 202, 154)}, paintGoat, 0},
        /* 22 Finlet     */ {{rgb(64, 184, 204), rgb(184, 238, 244), rgb(44, 122, 192), rgb(24, 90, 160)}, paintFish, 0},
        /* 23 Eclipsa    */ {{rgb(56, 44, 106), rgb(74, 60, 130), rgb(252, 232, 144), rgb(200, 170, 255)}, paintEclipse, 3},
        /* 24 Mossback   */ {{rgb(94, 144, 84), rgb(158, 188, 116), rgb(110, 170, 80), rgb(120, 170, 80)}, paintTurtle, 0},
        /* 25 Hailwing   */ {{rgb(228, 242, 255), rgb(246, 250, 255), rgb(132, 192, 242), rgb(132, 192, 242)}, paintIceBird, 0},
        /* 26 Magmaroth  */ {{rgb(76, 66, 72), rgb(110, 98, 100), rgb(255, 122, 36), rgb(255, 122, 36)}, paintLava, 3},
    };
    return table[std::clamp(line, 0, 26)];
}

}  // namespace

#include "sprite_gen.inc"

Sprite makeSprite(int speciesId, int frameIn) {
    // Frames 0 and 1 are the base pose (1 flutters tails, wings and flames and blinks); frames 2 and 3 are the same two
    // poses with the upper body leaning left (2) or right (3), which gives walking a waddle.
    const int frame = frameIn & 1;
    const int lean = frameIn >= 2 ? (frameIn == 2 ? -1 : 1) : 0;
    const Species& sp = species(speciesId);
    Cv c;
    int eyeY = 12, eyeA = 8;
    Pal pal;
    int eyeStyle = 0;
    if (sp.line >= 27) {
        const GenLine& gl = genLine(sp.line - 27);
        paintGeneric(c, sp.stage, frame, gl, eyeY, eyeA);
        pal = gl.pal;
        eyeStyle = gl.eyeStyle;
    } else {
        const LineInfo& li = lineInfo(sp.line);
        li.paint(c, sp.stage, frame, li.pal, eyeY, eyeA);
        pal = li.pal;
        eyeStyle = li.eyeStyle;
    }

    // Outline: darken the border of the painted shape.
    std::vector<u32> out = c.p;
    const u32 dark = mul(mix(pal.body, rgb(20, 14, 30), 0.65), 1.0);
    for (int y = 0; y < N; ++y)
        for (int x = 0; x < N; ++x) {
            if (c.get(x, y) != 0) continue;
            const int dx[4] = {1, -1, 0, 0}, dy[4] = {0, 0, 1, -1};
            for (int k = 0; k < 4; ++k)
                if (c.get(x + dx[k], y + dy[k]) != 0) { out[static_cast<size_t>(y * N + x)] = dark; break; }
        }
    c.p = out;
    // Eyes go on top of the outline.
    const int style = (frame == 1 && (speciesId % 3) != 0 && eyeStyle != 3) ? 1 : eyeStyle;
    eyes(c, eyeA, eyeY, style, pal.accent2);

    Sprite s;
    s.px = c.p;  // opaque or fully transparent, so premultiplication is a no-op
    if (lean != 0) {
        // Shift everything above the feet line one pixel sideways; the lower rows stay planted.
        const int split = 17;
        std::vector<u32> leaned(s.px.size(), 0);
        for (int y = 0; y < N; ++y)
            for (int x = 0; x < N; ++x) {
                const u32 v = s.px[static_cast<size_t>(y * N + x)];
                if (!v) continue;
                const int nx = (y < split) ? x + lean : x;
                if (nx >= 0 && nx < N) leaned[static_cast<size_t>(y * N + nx)] = v;
            }
        s.px = leaned;
    }
    return s;
}

Sprite makeSilhouette(const Sprite& src) {
    Sprite s = src;
    for (auto& p : s.px)
        if ((p >> 24) != 0) p = rgb(40, 43, 56);
    return s;
}

Sprite makeEggSprite(int frame) {
    Cv c;
    const u32 shell = rgb(246, 238, 206), spot = rgb(140, 190, 230), edge = rgb(90, 76, 64);
    const int sh = frame ? 1 : 0;
    for (int y = 3; y < 22; ++y) {
        const double t = (y - 12.5) / 9.5;
        const double w = 7.2 * std::sqrt(std::max(0.0, 1.0 - t * t)) * (y < 12 ? 0.88 : 1.0);
        for (int x = 0; x < N; ++x) {
            const double dx = std::fabs(x + 0.5 - 12 - sh * 0.5);
            if (dx <= w) c.set(x, y, ((x + y * 2) % 8 == 0 && dx < w - 1.5) ? spot : mul(shell, 1.08 - 0.25 * (y - 3) / 19.0));
        }
    }
    std::vector<u32> out = c.p;
    for (int y = 0; y < N; ++y) for (int x = 0; x < N; ++x) {
        if (c.get(x, y)) continue;
        const int dx[4] = {1, -1, 0, 0}, dy[4] = {0, 0, 1, -1};
        for (int k = 0; k < 4; ++k) if (c.get(x + dx[k], y + dy[k])) { out[static_cast<size_t>(y * N + x)] = edge; break; }
    }
    Sprite s;
    s.px = out;
    return s;
}

}  // namespace gf
