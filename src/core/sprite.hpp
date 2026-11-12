// Procedural placeholder pixel art. Each friend gets a deterministic 24x24 sprite built from a motif that matches
// its abilities (leaves for the regrowing tank, flames for the burst attacker, gears for the clockwork line, ...).
// Pixels are premultiplied ARGB (0xAARRGGBB), ready to upload to Direct2D or any other renderer.
#pragma once
#include <cstdint>
#include <vector>

namespace gf {

constexpr int kSpriteSize = 24;
constexpr int kSpriteFrames = 4;   // 0/1 base pose and flutter, 2/3 the same leaning left/right (see makeSprite)

struct Sprite {
    int w = kSpriteSize, h = kSpriteSize;
    std::vector<uint32_t> px;  // w*h
    uint32_t at(int x, int y) const { return px[static_cast<size_t>(y * w + x)]; }
};

Sprite makeSprite(int speciesId, int frame);
Sprite makeSilhouette(const Sprite& s);   // flat dark shape for undiscovered friends
Sprite makeEggSprite(int frame);          // frame 0 or 1 (wobble)

}  // namespace gf
