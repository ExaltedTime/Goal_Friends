// Procedural placeholder pixel art. Each friend gets a deterministic 16x16 sprite from its id.
// Pixels are premultiplied ARGB (0xAARRGGBB), ready to upload to Direct2D or any other renderer.
#pragma once
#include <cstdint>
#include <vector>

namespace gf {

struct Sprite {
    int w = 16, h = 16;
    std::vector<uint32_t> px;  // w*h
    uint32_t at(int x, int y) const { return px[static_cast<size_t>(y * w + x)]; }
};

constexpr int kSpriteFrames = 2;

Sprite makeSprite(int speciesId, int frame);
Sprite makeSilhouette(const Sprite& s);      // flat dark shape for undiscovered friends
Sprite makeEggSprite(int progressFrame);     // progressFrame 0 or 1 (wobble)

}  // namespace gf
