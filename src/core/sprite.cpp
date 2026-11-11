#include "sprite.hpp"

#include <algorithm>
#include <cmath>
#include <random>

#include "data.hpp"

namespace gf {
namespace {

struct RGB { double r, g, b; };

RGB hsl(double h, double s, double l) {
    h = std::fmod(h, 360.0) / 360.0;
    auto f = [&](double n) {
        const double k = std::fmod(n + h * 12.0, 12.0);
        const double a = s * std::min(l, 1.0 - l);
        return l - a * std::max(-1.0, std::min({k - 3.0, 9.0 - k, 1.0}));
    };
    return {f(0), f(8), f(4)};
}

uint32_t pack(RGB c, int a = 255) {
    auto q = [&](double v) { return static_cast<uint32_t>(std::clamp(v, 0.0, 1.0) * 255.0 + 0.5); };
    // premultiply
    const uint32_t A = static_cast<uint32_t>(a);
    const uint32_t R = q(c.r) * A / 255, G = q(c.g) * A / 255, B = q(c.b) * A / 255;
    return (A << 24) | (R << 16) | (G << 8) | B;
}

}  // namespace

Sprite makeSprite(int speciesId, int frame) {
    const Species& sp = species(speciesId);
    Sprite s;
    s.px.assign(static_cast<size_t>(s.w * s.h), 0);
    std::mt19937 rng(static_cast<uint32_t>(speciesId * 7919 + 17));
    auto rnd = [&](int n) { return static_cast<int>(rng() % static_cast<uint32_t>(n)); };
    auto rf = [&]() { return (rng() % 1000) / 1000.0; };

    const double hue = std::fmod(sp.line * 47.0 + (sp.rare ? 20.0 : 0.0), 360.0);
    const double light = sp.stage == 1 ? 0.56 : sp.stage == 2 ? 0.50 : 0.44;
    const RGB body = hsl(hue, 0.58, light);
    const RGB belly = hsl(hue + 10, 0.5, std::min(0.85, light + 0.22));
    const RGB dark = hsl(hue, 0.5, 0.16);
    const RGB accent = hsl(hue + 150, 0.7, 0.58);

    // Body shape, mirrored left to right.
    const double rx = 2.6 + sp.stage * 0.95 + (rnd(3) - 1) * 0.3;
    const double ry = 2.6 + sp.stage * 0.8 + (rnd(3) - 1) * 0.3;
    const double cy = 9.2 - (sp.stage - 1) * 0.3;
    std::vector<char> m(static_cast<size_t>(s.w * s.h), 0);
    auto set = [&](int x, int y, char v) {
        if (x < 0 || y < 0 || x >= s.w || y >= s.h) return;
        m[static_cast<size_t>(y * s.w + x)] = v;
        m[static_cast<size_t>(y * s.w + (s.w - 1 - x))] = v;
    };
    for (int y = 0; y < s.h; ++y)
        for (int x = 0; x < s.w / 2; ++x) {
            const double dx = (x - 7.5) / rx, dy = (y - cy) / ry;
            if (dx * dx + dy * dy <= 1.0 + (rf() - 0.5) * 0.45) set(x, y, 1);
        }
    // Features driven by the seed: ears, horn, feet, tail fin.
    const int ear = rnd(3);
    const int topY = static_cast<int>(cy - ry);
    if (ear == 0) { for (int i = 0; i < 3; ++i) { set(4, topY - i, 1); } set(5, topY, 1); }
    else if (ear == 1) { set(6, topY - 1, 1); set(6, topY - 2, 1); set(5, topY - 1, 1); }
    else { set(3, topY + 1, 1); set(3, topY, 1); set(4, topY - 1, 1); }
    const int feetY = std::min(s.h - 2, static_cast<int>(cy + ry));
    const int footShift = frame == 0 ? 0 : 1;
    set(5 - footShift, feetY + 1, 1);
    set(6 + footShift - (frame ? 1 : 0), feetY + 1, 2);
    if (sp.stage >= 2) set(2, static_cast<int>(cy) + 1, 1);                   // arms/tail
    if (sp.stage >= 3) { set(6, topY - 1, 3); set(6, topY - 2, 3); }           // crest in accent colour
    if (sp.rare) { set(3, topY + 2, 3); set(4, topY + 1, 3); }                 // sparkle trim

    // Belly patch and eyes.
    const int bx0 = 6, by0 = static_cast<int>(cy) + 1;
    for (int y = by0; y < by0 + 3; ++y)
        for (int x = bx0; x < 8; ++x)
            if (m[static_cast<size_t>(y * s.w + x)] == 1) set(x, y, 4);
    const int eyeY = static_cast<int>(cy) - 1;
    const int eyeX = 5 - (sp.stage >= 3 ? 0 : 0);

    // Paint
    auto colourFor = [&](char v) {
        switch (v) { case 4: return belly; case 3: return accent; case 2: return hsl(hue, 0.5, light - 0.08); default: return body; }
    };
    for (int y = 0; y < s.h; ++y)
        for (int x = 0; x < s.w; ++x) {
            const char v = m[static_cast<size_t>(y * s.w + x)];
            if (v) s.px[static_cast<size_t>(y * s.w + x)] = pack(colourFor(v));
        }
    // Outline
    std::vector<uint32_t> out = s.px;
    for (int y = 0; y < s.h; ++y)
        for (int x = 0; x < s.w; ++x) {
            if (m[static_cast<size_t>(y * s.w + x)]) continue;
            bool near = false;
            const int dx[4] = {1, -1, 0, 0}, dy[4] = {0, 0, 1, -1};
            for (int k = 0; k < 4; ++k) {
                const int nx = x + dx[k], ny = y + dy[k];
                if (nx >= 0 && ny >= 0 && nx < s.w && ny < s.h && m[static_cast<size_t>(ny * s.w + nx)]) near = true;
            }
            if (near) out[static_cast<size_t>(y * s.w + x)] = pack(dark);
        }
    s.px = out;
    // Eyes last so the outline doesn't cover them.
    auto put = [&](int x, int y, uint32_t c) {
        if (x >= 0 && y >= 0 && x < s.w && y < s.h) s.px[static_cast<size_t>(y * s.w + x)] = c;
    };
    const uint32_t white = pack({1, 1, 1}), ink = pack({0.05, 0.05, 0.1});
    for (int side = 0; side < 2; ++side) {
        const int x = side == 0 ? eyeX : s.w - 1 - eyeX;
        if (frame == 1 && (speciesId % 3) != 0) {
            put(x, eyeY + 1, ink);  // blink
        } else {
            put(x, eyeY, white);
            put(x, eyeY + 1, ink);
        }
    }
    return s;
}

Sprite makeSilhouette(const Sprite& src) {
    Sprite s = src;
    for (auto& p : s.px) if ((p >> 24) != 0) p = pack({0.16, 0.17, 0.22}, 255);
    return s;
}

Sprite makeEggSprite(int frame) {
    Sprite s;
    s.px.assign(static_cast<size_t>(s.w * s.h), 0);
    const uint32_t shell = pack({0.96, 0.92, 0.80}), spot = pack({0.55, 0.75, 0.9}), edge = pack({0.35, 0.3, 0.25});
    const int shift = frame ? 1 : 0;
    for (int y = 2; y < 15; ++y) {
        const double t = (y - 8.5) / 6.5;
        const double w = 5.2 * std::sqrt(std::max(0.0, 1.0 - t * t)) * (y < 8 ? 0.85 : 1.0);
        for (int x = 0; x < s.w; ++x) {
            const double dx = std::fabs(x + shift - 7.5);
            if (dx <= w) {
                const bool sp = ((x + y * 2) % 7 == 0) && dx < w - 1;
                s.px[static_cast<size_t>(y * s.w + x)] = sp ? spot : shell;
            } else if (dx <= w + 1) {
                s.px[static_cast<size_t>(y * s.w + x)] = edge;
            }
        }
    }
    return s;
}

}  // namespace gf
