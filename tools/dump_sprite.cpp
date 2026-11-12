// Prints a friend's sprite as "x y r g b a" lines (used by tools/make_icon.py). Usage: gf_dump_sprite <speciesId> [frame]
#include <cstdio>
#include <cstdlib>
#include "sprite.hpp"
int main(int argc, char** argv) {
    const int id = argc > 1 ? std::atoi(argv[1]) : 0, frame = argc > 2 ? std::atoi(argv[2]) : 0;
    const gf::Sprite s = gf::makeSprite(id, frame);
    for (int y = 0; y < s.h; ++y)
        for (int x = 0; x < s.w; ++x) {
            const uint32_t p = s.at(x, y);
            std::printf("%d %d %u %u %u %u\n", x, y, (p >> 16) & 255, (p >> 8) & 255, p & 255, p >> 24);
        }
}
