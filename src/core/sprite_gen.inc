// Included by sprite.cpp (shares its canvas helpers); not a separate translation unit.
namespace {

struct Geo { double cy = 15, rx = 6, ry = 5, top = 10, bot = 20; int st = 1, f = 0; };

u32 palCol(const Pal& P, int idx) {
    switch (idx) {
        case 0: return P.body;
        case 1: return P.belly;
        case 2: return P.accent;
        case 3: return P.accent2;
        case 4: return rgb(250, 250, 255);
        default: return mul(P.body, 0.35);
    }
}

bool isBackPart(PartK k) {
    switch (k) {
        case PartK::TailFluffy: case PartK::TailFish: case PartK::TailFlame: case PartK::TailBolt: case PartK::TailLong: case PartK::TailLeaf:
        case PartK::WingsInsect: case PartK::WingsFeather: case PartK::WingsMembrane: case PartK::WingsFlame:
        case PartK::Mane: case PartK::Cloud: case PartK::ShellBack: case PartK::Moon: case PartK::Scythe:
            return true;
        default: return false;
    }
}
bool isOverlayPart(PartK k) {
    return k == PartK::Stripes || k == PartK::Spots || k == PartK::Plates || k == PartK::Cracks || k == PartK::Warts || k == PartK::Runes;
}

void drawBody(Cv& c, BodyK b, int st, int f, const Pal& P, Geo& g) {
    g.st = st; g.f = f;
    switch (b) {
        case BodyK::Round:
            g.rx = 4.4 + 0.95 * st; g.ry = 4.2 + 0.85 * st; g.cy = 17.2 - 0.5 * st;
            c.ell(12, g.cy, g.rx, g.ry, P.body);
            c.ell(12, g.cy + g.ry * 0.35, g.rx * 0.6, g.ry * 0.55, P.belly);
            break;
        case BodyK::Tall:
            g.rx = 3.9 + 0.8 * st; g.ry = 5.6 + 0.95 * st; g.cy = 15.6 - 0.3 * st;
            c.ell(12, g.cy, g.rx, g.ry, P.body);
            c.ell(12, g.cy + g.ry * 0.3, g.rx * 0.6, g.ry * 0.5, P.belly);
            break;
        case BodyK::Wide:
            g.rx = 5.8 + 1.2 * st; g.ry = 3.6 + 0.75 * st; g.cy = 17.6 - 0.3 * st;
            c.ell(12, g.cy, g.rx, g.ry, P.body);
            c.ell(12, g.cy + g.ry * 0.35, g.rx * 0.6, g.ry * 0.5, P.belly);
            break;
        case BodyK::Pear: {
            const double bry = 4.0 + 0.7 * st, bcy = 18.6 - 0.4 * st;
            c.ell(12, bcy, 4.6 + 0.9 * st, bry, P.body);
            c.ell(12, bcy + 1, 3.0 + 0.5 * st, bry * 0.6, P.belly);
            g.rx = 3.2 + 0.4 * st; g.ry = 3.0 + 0.4 * st; g.cy = bcy - bry - 0.6;
            c.ell(12, g.cy, g.rx, g.ry, P.body);
            break;
        }
        case BodyK::Serpent: {
            for (int i = 4; i >= 1; --i) {   // tail segments first, then the head on top
                const double x = 12 + (i % 2 ? 3.2 : -3.2) * (i == 4 ? 0.6 : 1.0), y = 8.5 + i * (3.0 + 0.15 * st), r = 3.2 + 0.35 * st - 0.35 * i + 0.4;
                c.ell(x, y, std::max(1.8, r), std::max(1.8, r), mul(P.body, 1.0 - 0.04 * i));
            }
            g.rx = 3.6 + 0.5 * st; g.ry = 3.3 + 0.45 * st; g.cy = 9.0 + 0.2 * st;
            c.ell(12, g.cy, g.rx, g.ry, P.body);
            c.ell(12, g.cy + 1.4, g.rx * 0.55, g.ry * 0.4, P.belly);
            break;
        }
        case BodyK::Dome: {
            g.rx = 5.6 + 1.1 * st; g.ry = 5.0 + 0.9 * st; g.cy = 17.4 - 0.2 * st;
            c.ell(12, g.cy, g.rx, g.ry, P.body);
            for (int y = static_cast<int>(g.cy + g.ry * 0.45); y < N; ++y) for (int x = 0; x < N; ++x) c.p[static_cast<size_t>(y * N + x)] = 0;   // flat base
            c.rect(static_cast<int>(12 - g.rx + 0.5), static_cast<int>(g.cy + g.ry * 0.45) - 1, static_cast<int>(12 + g.rx - 0.5), static_cast<int>(g.cy + g.ry * 0.45) - 1, mul(P.body, 0.8));
            c.ell(12, g.cy + 0.5, g.rx * 0.55, g.ry * 0.4, P.belly);
            g.ry = g.ry * 0.7;  // visible height
            break;
        }
        case BodyK::Blob: {
            g.rx = 5.0 + 1.0 * st; g.ry = 4.4 + 0.8 * st; g.cy = 15.8 - 0.4 * st;
            c.ell(12, g.cy, g.rx, g.ry, P.body);
            for (int x = static_cast<int>(12 - g.rx); x < static_cast<int>(12 + g.rx); ++x) {        // wavy underside
                const int h = ((x + f) % 3 == 0) ? 2 : 1;
                for (int k = 0; k < h; ++k) c.set(x, static_cast<int>(g.cy + g.ry) + k, mul(P.body, 0.85));
            }
            c.ell(12, g.cy + 1.2, g.rx * 0.6, g.ry * 0.5, P.belly);
            break;
        }
        case BodyK::Wisp: {
            g.rx = 4.6 + 0.9 * st; g.ry = 5.2 + 0.7 * st; g.cy = 12.0 + (f ? 0.5 : 0);
            c.ell(12, g.cy, g.rx, g.ry, P.body);
            c.tri(12 - g.rx + 1, g.cy + 2.5, 12, 22.5 - f, 12 + g.rx - 1, g.cy + 2.5, mul(P.body, 0.95));
            c.ell(12, g.cy + 1.4, g.rx * 0.55, g.ry * 0.5, P.belly);
            break;
        }
    }
    g.top = g.cy - g.ry;
    g.bot = g.cy + g.ry;
}

void drawPart(Cv& c, const PartSpec& ps, const Geo& g, const Pal& P) {
    const u32 col = palCol(P, ps.col), dk = mul(col, 0.72), lt = mix(col, rgb(255, 255, 255), 0.4);
    const double T = g.top, B = g.bot, rx = g.rx, cy = g.cy;
    const int st = g.st, f = g.f;
    const int ti = static_cast<int>(T), bi = static_cast<int>(B);
    switch (ps.k) {
        case PartK::EarsPoint:
            c.mtri(12 - rx * 0.75, T + 2.2, 12 - rx * 0.95, T - 3.4 - st * 0.7, 12 - rx * 0.1, T + 1.2, col);
            c.mtri(12 - rx * 0.68, T + 1.6, 12 - rx * 0.85, T - 1.4 - st * 0.4, 12 - rx * 0.35, T + 1.0, P.belly);
            break;
        case PartK::EarsRound:
            c.dot(12 - rx * 0.72, T + 0.6, 2.0 + 0.4 * st, col); c.dot(12 + rx * 0.72, T + 0.6, 2.0 + 0.4 * st, col);
            c.dot(12 - rx * 0.72, T + 0.8, 1.0 + 0.2 * st, P.belly); c.dot(12 + rx * 0.72, T + 0.8, 1.0 + 0.2 * st, P.belly);
            break;
        case PartK::EarsLong:
            for (int s = -1; s <= 1; s += 2) {
                c.ell(12 + s * rx * 0.42, T - 2.4 - st * 0.7, 1.6, 3.8 + st * 0.7, col);
                c.ell(12 + s * rx * 0.42, T - 2.2 - st * 0.7, 0.8, 2.6 + st * 0.5, P.belly, false);
            }
            break;
        case PartK::HornsCurl: {
            const int x0 = static_cast<int>(12 - rx * 0.55);
            c.mline(x0, ti + 1, x0 - 2, ti - 2, col); c.mline(x0 - 2, ti - 2, x0 - 3, ti - 4 - st / 2, col); c.mline(x0 - 3, ti - 4 - st / 2, x0 - 1, ti - 5 - st / 2, lt);
            c.m(x0 - 1, ti, dk);
            break;
        }
        case PartK::HornsStraight:
            c.mtri(12 - rx * 0.55, T + 1.5, 12 - rx * 0.9, T - 3.4 - st * 0.5, 12 - rx * 0.2, T + 0.5, col);
            break;
        case PartK::Antlers: {
            const int x0 = static_cast<int>(12 - rx * 0.45);
            c.mline(x0, ti + 1, x0 - 1, ti - 4 - st, col); c.mline(x0 - 1, ti - 2, x0 - 4, ti - 3 - st, col); c.mline(x0 - 1, ti - 4 - st, x0 - 3, ti - 6 - st, col); c.mline(x0, ti - 1, x0 + 1, ti - 3 - st / 2, col);
            break;
        }
        case PartK::Antennae:
            c.mline(10, ti + 1, 8 - st / 2, ti - 3 - st / 2, col); c.m(7 - st / 2, ti - 4 - st / 2, lt); c.m(8 - st / 2, ti - 4 - st / 2, lt);
            break;
        case PartK::TailFluffy:
            c.ell(12 + rx + 1.6, B - 3.2, 2.4 + 0.4 * st, 3.2 + 0.4 * st, col); c.dot(12 + rx + 1.6, B - 5, 1.1, lt);
            break;
        case PartK::TailFish:
            c.tri(12 + rx - 1, B - 4, 12 + rx + 4.6, B - 8 - f, 12 + rx + 4.6, B - 1, col); c.tri(12 + rx - 1, B - 3.6, 12 + rx + 3, B - 6 - f, 12 + rx + 3, B - 2, lt);
            break;
        case PartK::TailFlame:
            c.tri(12 + rx - 1, B - 2, 12 + rx + 3.6, B - 9 - st - f, 12 + rx + 3.4, B - 1, col); c.tri(12 + rx, B - 2, 12 + rx + 2.6, B - 6 - st - f, 12 + rx + 3, B - 1, lt);
            break;
        case PartK::TailBolt: {
            const int x = static_cast<int>(12 + rx), y = bi - 3;
            c.line(x, y, x + 3, y - 3, col); c.line(x + 3, y - 3, x + 1 + f, y - 5, col); c.line(x + 1 + f, y - 5, x + 4, y - 9, col); c.line(x + 4, y - 9, x + 4, y - 10, lt);
            break;
        }
        case PartK::TailLong: {
            const int x = static_cast<int>(12 + rx), y = bi - 3;
            c.line(x, y, x + 3, y - 2 + f, col); c.line(x + 3, y - 2 + f, x + 4, y - 6, col); c.dot(x + 4, y - 7, 1.4, lt);
            break;
        }
        case PartK::TailLeaf:
            c.tri(12 + rx - 1, B - 2, 12 + rx + 4, B - 9 - f, 12 + rx + 5, B - 3, col); c.line(static_cast<int>(12 + rx), bi - 2, static_cast<int>(12 + rx + 3), bi - 7, dk);
            break;
        case PartK::WingsInsect:
            for (int s = -1; s <= 1; s += 2) {
                c.ell(12 + s * (rx + 2.6), cy - 3 - f, 3.2 + st * 0.6, 5.2 + st * 0.8, col, false);
                c.ell(12 + s * (rx + 2.2), cy + 1.5 + f * 0.5, 2.4 + st * 0.4, 3.2 + st * 0.5, lt, false);
            }
            break;
        case PartK::WingsFeather:
            c.mtri(12 - rx + 1, cy - 1, 12 - rx - 4 - st * 1.5, cy - 6 - f, 12 - rx - 1, cy + 4, col);
            c.mtri(12 - rx + 1, cy + 1, 12 - rx - 3 - st, cy + 2 + f, 12 - rx - 1, cy + 6, dk);
            c.mline(static_cast<int>(12 - rx), static_cast<int>(cy), static_cast<int>(12 - rx - 3 - st), static_cast<int>(cy - 4), lt);
            break;
        case PartK::WingsMembrane:
            c.mtri(12 - rx + 1, cy - 2, 12 - rx - 5 - st, cy - 7 - f, 12 - rx - 1, cy + 4, col);
            c.mline(static_cast<int>(12 - rx), static_cast<int>(cy - 2), static_cast<int>(12 - rx - 5 - st), static_cast<int>(cy - 7 - f), dk);
            c.mline(static_cast<int>(12 - rx), static_cast<int>(cy), static_cast<int>(12 - rx - 4 - st), static_cast<int>(cy + 1), dk);
            break;
        case PartK::WingsFlame:
            c.mtri(12 - rx + 1, cy - 1, 12 - rx - 4 - st, cy - 8 - f, 12 - rx - 1, cy + 4, col);
            c.mtri(12 - rx + 1, cy, 12 - rx - 3 - st, cy - 5 - f, 12 - rx - 1, cy + 3, lt);
            break;
        case PartK::CrestLeaf:
            c.ell(12 - 2.4, T - 0.8 - f * 0.5, 2.6, 1.5, col, false); c.ell(12 + 2.4, T - 0.8 - f * 0.5, 2.6, 1.5, col, false); c.rect(11, ti - 1, 12, ti + 1, dk);
            if (st >= 2) { c.ell(12 - 1.6, T - 3, 2.2, 1.3, lt, false); c.ell(12 + 1.6, T - 3, 2.2, 1.3, lt, false); }
            break;
        case PartK::CrestFlame:
            c.tri(10, T + 1, 12, T - 5 - st - f, 14, T + 1, col); c.tri(10.8, T + 1, 12, T - 2.5 - st - f, 13.2, T + 1, lt);
            if (st >= 2) c.mtri(9, T + 1.5, 7.5, T - 2 - f, 10.5, T + 1, col);
            break;
        case PartK::CrestIce:
            for (int i = 0; i < 2 + st; ++i) c.tri(8.5 + i * 2.0, T + 1.5, 9.5 + i * 2.0, T - 3.5 - (i % 2) - st * 0.4, 10.5 + i * 2.0, T + 1.5, i % 2 ? lt : col);
            break;
        case PartK::CrestGem:
            c.tri(9.8, T - 2, 12, T - 6 - st * 0.4, 14.2, T - 2, col); c.tri(9.8, T - 2, 12, T + 0.8, 14.2, T - 2, dk); c.set(11, ti - 4, rgb(255, 255, 255));
            break;
        case PartK::CrestStar: {
            const int y = ti - 3 - st / 2;
            c.set(11, y, col); c.set(12, y, col); c.set(11, y - 1, lt); c.set(12, y - 1, lt); c.set(11, y + 1, col); c.set(12, y + 1, col); c.m(9, y, lt); c.m(10, y, col);
            break;
        }
        case PartK::CrestFeather:
            for (int i = 0; i < 3; ++i) c.line(10 + i * 2, ti + 1, 9 + i * 3, ti - 4 - st / 2 - (i == 1), i == 1 ? lt : col);
            break;
        case PartK::Spikes:
            for (int i = 0; i < st + 1; ++i) c.mtri(12 - rx + 0.5, cy - g.ry * 0.2 + i * 2.5, 12 - rx - 2.2 - 0.4 * st, cy - g.ry * 0.35 + i * 2.5, 12 - rx + 0.8, cy + g.ry * 0.15 + i * 2.5, col);
            break;
        case PartK::Fins:
            c.mtri(12 - rx + 0.5, cy - 1, 12 - rx - 3.6, cy - 3 - f, 12 - rx + 0.5, cy + 3, col);
            c.tri(10, T + 1, 12, T - 3 - f, 14, T + 1, col);
            break;
        case PartK::Claws: {
            const int x0 = static_cast<int>(12 - rx) - 1;
            c.mrect(x0, bi - 4, x0 + 2, bi - 1, mul(P.body, 0.9));
            c.m(x0 - 1, bi, col); c.m(x0 + 1, bi + 1, col); c.m(x0 + 3, bi, col);
            break;
        }
        case PartK::Arms: {
            const int x = static_cast<int>(12 - rx);
            c.mrect(x - 3, static_cast<int>(cy) - 1, x, static_cast<int>(cy), col);
            c.mrect(x - 3, static_cast<int>(cy) - 5, x - 2, static_cast<int>(cy) - 1, col);
            break;
        }
        case PartK::Stripes:
            for (double fr : {-0.35, 0.1, 0.55}) { const int y = static_cast<int>(cy + g.ry * fr); for (int x = 0; x < N; ++x) if (c.get(x, y)) c.set(x, y, col); }
            break;
        case PartK::Spots: {
            const double pos[4][2] = {{-0.45, -0.3}, {0.4, -0.1}, {-0.1, 0.4}, {0.5, 0.5}};
            for (auto& p : pos) { const int x = static_cast<int>(12 + p[0] * rx), y = static_cast<int>(cy + p[1] * g.ry); if (c.get(x, y)) { c.set(x, y, col); if (c.get(x + 1, y)) c.set(x + 1, y, col); } }
            break;
        }
        case PartK::Mane:
            c.ell(12, cy - g.ry * 0.55, rx + 2.6, g.ry * 0.85, col, false);
            break;
        case PartK::Halo:
            for (int a = 0; a < 16; ++a) { const double t = a * 6.2832 / 16; c.set(static_cast<int>(12 + std::cos(t) * 4.2), static_cast<int>(T - 3 - st / 2 + std::sin(t) * 1.4), a % 2 ? lt : col); }
            break;
        case PartK::Crown:
            c.rect(9, ti - 1, 14, ti, col);
            c.tri(9, ti - 1, 9.6, ti - 4, 10.8, ti - 1, col); c.tri(11, ti - 1, 12, ti - 5, 13, ti - 1, col); c.tri(13.2, ti - 1, 14.4, ti - 4, 15, ti - 1, col);
            c.set(11, ti - 1, lt);
            break;
        case PartK::Bubbles:
            c.dot(12 + rx + 2, cy - 4 - f, 1.3, lt); c.dot(12 - rx - 2, cy - 6 + f, 1.0, lt); c.dot(12 + rx + 1, cy - 8, 0.9, col);
            break;
        case PartK::Sparkles: {
            const int sp = f ? 1 : 0;
            const int pts[3][2] = {{3 + sp, 5}, {20 - sp, 9}, {4, 18 - sp}};
            for (auto& p : pts) { c.set(p[0], p[1], rgb(255, 255, 255)); c.set(p[0] - 1, p[1], col); c.set(p[0] + 1, p[1], col); c.set(p[0], p[1] - 1, col); c.set(p[0], p[1] + 1, col); }
            break;
        }
        case PartK::Plates:
            for (int i = 0; i < 3; ++i) {
                const int y = static_cast<int>(cy - g.ry * 0.5 + i * g.ry * 0.5);
                for (int x = 0; x < N; ++x) if (c.get(x, y)) c.set(x, y, mul(col, 0.65));
                for (int x = 0; x < N; ++x) if (c.get(x, y - 1)) c.set(x, y - 1, lt);
            }
            break;
        case PartK::Cracks: {
            const int cx0 = static_cast<int>(12 - rx * 0.4), y0 = static_cast<int>(cy - g.ry * 0.4);
            c.line(cx0, y0, cx0 + 2, y0 + 3, col); c.line(cx0 + 2, y0 + 3, cx0 + 1, y0 + 6, col);
            c.line(static_cast<int>(12 + rx * 0.45), y0 + 1, static_cast<int>(12 + rx * 0.3), y0 + 4 + f, col);
            break;
        }
        case PartK::Warts:
            for (int i = -1; i <= 1; ++i) c.dot(12 + i * 2.8, T + 2.0 + (i == 0 ? 0 : 1.2), 0.9, col);
            break;
        case PartK::ShellBack:
            c.ell(13, cy - 2, rx + 1.2, g.ry + 0.6, col);
            c.ell(13, cy - 2, rx * 0.6, g.ry * 0.5, dk, false); c.ell(13, cy - 2, rx * 0.25, g.ry * 0.2, col, false);
            break;
        case PartK::Cloud:
            c.dot(12 - rx, cy - g.ry * 0.5, 3 + st * 0.5, col); c.dot(12 + rx, cy - g.ry * 0.5, 3 + st * 0.5, col); c.dot(12, T - 0.5, 3.6 + st * 0.5, col); c.dot(12 - rx * 0.5, T + 0.5, 3, col); c.dot(12 + rx * 0.5, T + 0.5, 3, col);
            break;
        case PartK::Flower:
            for (int i = 0; i < 5; ++i) { const double t = i * 1.2566 + 0.3; c.dot(12 + std::cos(t) * 3.0, T - 1.5 + std::sin(t) * 2.0, 1.1, col); }
            c.dot(12, T - 1.5, 1.0, rgb(255, 214, 90));
            break;
        case PartK::Bell:
            c.rect(static_cast<int>(12 - rx), bi - 2, static_cast<int>(12 + rx) - 1, bi - 1, col);
            c.dot(12, B + 0.5, 1.3, mul(col, 0.8)); c.m(static_cast<int>(12 - rx) - 2, static_cast<int>(cy), lt); c.m(static_cast<int>(12 - rx) - 3, static_cast<int>(cy) - 2, lt);
            break;
        case PartK::Lantern: {
            c.line(11, ti, 14, ti - 4 - st / 2, dk);
            const int lx = 14, ly = ti - 7 - st / 2;
            c.rect(lx - 1, ly, lx + 1, ly + 2, col); c.set(lx, ly + 1, rgb(255, 255, 220)); c.set(lx, ly - 1, dk);
            break;
        }
        case PartK::Moon:
            c.ell(12, T + 1, 8, 8, col, false); c.ell(14.5, T, 7, 7.3, rgb(1, 2, 3), false);
            for (int y = 0; y < N; ++y) for (int x = 0; x < N; ++x) if (c.get(x, y) == rgb(1, 2, 3)) c.p[static_cast<size_t>(y * N + x)] = 0;
            break;
        case PartK::Notes: {
            const int pts[2][2] = {{3, 6 + f}, {20, 9 - f}};
            for (auto& p : pts) { c.line(p[0], p[1], p[0], p[1] + 3, col); c.set(p[0] - 1, p[1] + 3, col); c.set(p[0] - 1, p[1] + 4, col); c.set(p[0] + 1, p[1], col); c.set(p[0] + 1, p[1] + 1, col); }
            break;
        }
        case PartK::Fangs:
            c.set(10, static_cast<int>(cy + g.ry * 0.38), rgb(255, 255, 255)); c.set(13, static_cast<int>(cy + g.ry * 0.38), rgb(255, 255, 255)); c.set(10, static_cast<int>(cy + g.ry * 0.38) + 1, rgb(255, 255, 255)); c.set(13, static_cast<int>(cy + g.ry * 0.38) + 1, rgb(255, 255, 255));
            break;
        case PartK::Whiskers:
            c.mline(static_cast<int>(12 - rx) - 2, static_cast<int>(cy), static_cast<int>(12 - rx) + 2, static_cast<int>(cy), dk); c.mline(static_cast<int>(12 - rx) - 2, static_cast<int>(cy) + 2, static_cast<int>(12 - rx) + 2, static_cast<int>(cy) + 1, dk);
            break;
        case PartK::Beak:
            c.tri(10.5, cy + 0.6, 12, cy + 4, 13.5, cy + 0.6, col);
            break;
        case PartK::Stinger:
            c.tri(11, B - 1, 12, B + 3.2, 13, B - 1, col);
            break;
        case PartK::Magnet:
            c.rect(10, ti - 5, 14, ti - 4, mix(P.accent, P.accent2, 0.5)); c.rect(10, ti - 4, 11, ti, P.accent); c.rect(13, ti - 4, 14, ti, P.accent2); c.set(10, ti - 5, rgb(255, 255, 255));
            break;
        case PartK::Tentacles:
            for (int i : {-3, -1, 1, 3}) { const int x = 12 + i; for (int k = 0; k < 4 + st; ++k) c.set(x + (((k + f + i) / 2) % 2), bi + k, k % 3 == 0 ? lt : col); }
            break;
        case PartK::Runes: {
            const int pts[3][2] = {{static_cast<int>(12 - rx * 0.4), static_cast<int>(cy - 1)}, {static_cast<int>(12 + rx * 0.3), static_cast<int>(cy + 1)}, {static_cast<int>(12), static_cast<int>(cy + g.ry * 0.5)}};
            int k = 0;
            for (auto& p : pts) { if (!c.get(p[0], p[1])) { ++k; continue; } c.set(p[0], p[1], col); c.set(p[0], p[1] + 1, col); if (k == 0) c.set(p[0] + 1, p[1], col); if (k == 1) c.set(p[0] - 1, p[1] + 1, col); ++k; }
            break;
        }
        case PartK::Scythe:
            c.line(static_cast<int>(12 + rx) + 1, bi, static_cast<int>(12 + rx) + 3, ti - 2, dk);
            c.tri(12 + rx + 3, T - 2, 12 + rx - 2.5, T - 5, 12 + rx + 1, T + 1, col);
            break;
    }
}

struct GenLine {
    Pal pal;
    BodyK body[3];
    PartSpec parts[kMaxParts];
    int nparts;
    int eyeStyle;
};

void paintGeneric(Cv& c, int stage, int f, const GenLine& gl, int& eyeY, int& eyeA) {
    Geo g;
    // Geometry is needed by the back parts before the body is drawn, so measure on a scratch canvas first.
    {
        Cv scratch;
        drawBody(scratch, gl.body[std::clamp(stage - 1, 0, 2)], stage, f, gl.pal, g);
    }
    for (int i = 0; i < gl.nparts; ++i) if (stage >= gl.parts[i].minStage && isBackPart(gl.parts[i].k)) drawPart(c, gl.parts[i], g, gl.pal);
    drawBody(c, gl.body[std::clamp(stage - 1, 0, 2)], stage, f, gl.pal, g);
    for (int i = 0; i < gl.nparts; ++i) if (stage >= gl.parts[i].minStage && isOverlayPart(gl.parts[i].k)) drawPart(c, gl.parts[i], g, gl.pal);
    for (int i = 0; i < gl.nparts; ++i) if (stage >= gl.parts[i].minStage && !isBackPart(gl.parts[i].k) && !isOverlayPart(gl.parts[i].k)) drawPart(c, gl.parts[i], g, gl.pal);
    eyeA = std::clamp(static_cast<int>(12 - g.rx * 0.55), 6, 10);
    eyeY = static_cast<int>(g.cy - g.ry * 0.22);
}

#include "sprite_lines.inc"

}  // namespace
