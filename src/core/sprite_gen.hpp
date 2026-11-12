// Parametric sprite parts for the expanded roster: a body shape plus a list of themed parts (horns, wings, tails,
// crests, ...). Each line of friends picks a body per evolution stage and a few parts, so they look like what they do.
#pragma once
#include <cstdint>

namespace gf {

enum class BodyK : uint8_t { Round, Tall, Wide, Pear, Serpent, Dome, Blob, Wisp };

enum class PartK : uint8_t {
    EarsPoint, EarsRound, EarsLong, HornsCurl, HornsStraight, Antlers, Antennae,
    TailFluffy, TailFish, TailFlame, TailBolt, TailLong, TailLeaf,
    WingsInsect, WingsFeather, WingsMembrane, WingsFlame,
    CrestLeaf, CrestFlame, CrestIce, CrestGem, CrestStar, CrestFeather,
    Spikes, Fins, Claws, Arms, Stripes, Spots, Mane, Halo, Crown, Bubbles, Sparkles, Plates, Cracks, Warts,
    ShellBack, Cloud, Flower, Bell, Lantern, Moon, Notes, Fangs, Whiskers, Beak, Stinger, Magnet, Tentacles, Runes, Scythe
};

// Colour slots: 0 body, 1 belly, 2 accent, 3 accent2, 4 white, 5 dark.
struct PartSpec { PartK k; uint8_t col; uint8_t minStage; };

constexpr int kMaxParts = 8;

}  // namespace gf
