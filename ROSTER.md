# Goal Friends — Roster (v0.4)

150 friends (31 original + 17 in v0.3 + 102 in v0.4), original designs inspired by Pokémon. **No types.** Each has 4 unique moves (no move name is
reused). Effect keywords (Wound, Daze, Armor, Charge, ...) are defined in [DESIGN.md](DESIGN.md) section 5.

Stat pips are relative (1 = low, 5 = high): HP / ATK / DEF / SPD.
Move format: **Name** (cooldown) — effect. No cooldown listed means 0.
Power tiers: Light ~40, Medium ~70, Heavy ~110.

Battles are **triples**: 3 friends active per side from a team of 6. Targeting tags:
**[Ally]** = can be aimed at any active ally (default is self-only for buffs); **Spread** = hits all enemies at
75% damage; **Random targets** = each hit picks a random enemy.

Evolution: 3-stage lines evolve at levels 16 and 32. 2-stage lines evolve at level 20. Rares don't evolve.

---

## Starter lines (pick one at launch)

### Line S1 — Stalwart (tank / stall)

**1. Mossling** (stage 1) — HP 4 · ATK 2 · DEF 3 · SPD 1
- **Leaf Slap** — Light hit.
- **Sap Root** (CD 3) — Regen 3 on self.
- **Bramble Coat** (CD 3) — Thorns 3 on self.
- **Sprout Nap** (CD 4) — Heal 30% HP, then Slow 1 on self.

**2. Thornback** (stage 2) — HP 4 · ATK 3 · DEF 4 · SPD 1
- **Spike Roll** — Medium hit; 30% chance of Wound 2.
- **Ironbark** (CD 3) — Armor 3 on self.
- **Barbed Embrace** (CD 3) — Medium hit; Trap 2.
- **Photosynth** (CD 4) — Heal 30% HP.

**3. Grovewarden** (stage 3) — HP 5 · ATK 3 · DEF 5 · SPD 1
- **Ancient Crush** (CD 2) — Heavy hit; Slow 1 on self.
- **Verdant Bulwark** (CD 3) — Shield 20% max HP and Thorns 2.
- **Overgrowth** (CD 4) — Regen 4 on self and Cleanse.
- **Warden's Judgment** (CD 3) — Medium hit; +50% damage if you have Armor, Shield or Thorns active.

### Line S2 — Swift (speed / tempo)

**4. Zapling** (stage 1) — HP 2 · ATK 3 · DEF 1 · SPD 5
- **Static Nip** — Light hit; Priority.
- **Hyper Dash** (CD 3) — Haste 3 on self.
- **Jolt Rattle** (CD 3) — Light hit; Daze 2.
- **Quick Charge** (CD 3) — Focus on self; Priority.

**5. Zephox** (stage 2) — HP 3 · ATK 3 · DEF 2 · SPD 5
- **Slipstream Strike** — Medium hit; +30% damage if you moved first this turn.
- **Afterimage** (CD 3) — Evade; Priority.
- **Spark Chain** (CD 3) — Two Light hits at random targets; each has a 20% chance of Daze 1.
- **Windup Zip** (CD 3) — Haste 2 and Focus on self.

**6. Tempestail** (stage 3) — HP 3 · ATK 4 · DEF 2 · SPD 5
- **Thunder Lash** (CD 2) — Heavy hit; ignores 30% of target DEF.
- **Eye of the Storm** (CD 4) — Evade; your next attack deals +60% damage.
- **Rolling Thunder** (CD 2) — Three hits at power 30 / 50 / 70.
- **Gale Veil** (CD 3) — Haste 3 on self and Cleanse.

### Line S3 — Ember (burst / setup)

**7. Cindrel** (stage 1) — HP 2 · ATK 4 · DEF 1 · SPD 3
- **Ember Snap** — Light hit; 25% chance of Wound 2.
- **Kindle** (CD 3) — Focus on self.
- **Flare Burst** (CD 2) — Heavy hit; you take 10% of max HP recoil.
- **Smolder Aura** (CD 4) — Wound 3 and Mark on target.

**8. Ashwolf** (stage 2) — HP 3 · ATK 4 · DEF 2 · SPD 3
- **Cinder Fang** — Medium hit; 30% chance of Wound 2.
- **Ashen Howl** (CD 3) — Your ATK +30% for 3 turns.
- **Scorched Earth** (CD 4) — Wound 3 and Slow 2 on target.
- **Backdraft** (CD 2) — Heavy hit; +50% damage if target is Wounded.

**9. Pyrelord** (stage 3) — HP 3 · ATK 5 · DEF 2 · SPD 3
- **Inferno Crown** (CD 2) — Heavy hit; Wound 2.
- **Phoenix Pact** (CD 5) — Lose 25% HP; your next two attacks deal +80% damage.
- **Ember Rain** (CD 2) — Three Light hits at random targets; each has a 25% chance of Wound 2.
- **Molten Guard** (CD 3) — Counter: retaliate with a Medium hit plus Wound 2.

---

## Two-stage lines (evolve at level 20)

### Line L1 — Wall / counter

**10. Pebblet** — HP 3 · ATK 2 · DEF 4 · SPD 1
- **Rock Toss** — Light hit.
- **Curl Up** (CD 3) — Armor 2 on self; Counter (1.5x damage taken).
- **Gravel Spray** (CD 3) — Spread Light hit; Daze 1.
- **Settle In** (CD 3) — Heal 15% HP and Shield 10% max HP.

**11. Boulderon** — HP 4 · ATK 3 · DEF 5 · SPD 1
- **Landslide** (CD 2) — Heavy hit; Slow 2 on target.
- **Stonewall Stance** (CD 3) — Counter: retaliate for 2x the damage taken.
- **Tremor Step** (CD 3) — Medium hit; Seal.
- **Bedrock Resolve** (CD 4) — Shield 30% max HP; Slow 1 on self.

### Line L2 — Healer / cleanser

**12. Dewdrip** — HP 3 · ATK 1 · DEF 2 · SPD 3
- **Bubble Pop** — Light hit.
- **Mending Mist** (CD 3) [Ally] — Heal 20% HP.
- **Rinse** (CD 3) [Ally] — Cleanse; Regen 2.
- **Slick Skin** (CD 3) — 50% chance to Evade.

**13. Tidalisk** — HP 4 · ATK 2 · DEF 3 · SPD 3
- **Crashing Wave** (CD 2) — Heavy hit; Cleanse.
- **Healing Current** (CD 3) [Ally] — Heal 35% HP.
- **Undertow** — Medium hit; Slow 2.
- **Tidal Barrier** (CD 4) [Ally] — Shield 25% max HP; Regen 2.

### Line L3 — Debuffer / curse

**14. Gloomoth** — HP 2 · ATK 2 · DEF 2 · SPD 4
- **Dust Flutter** — Light hit; Daze 1.
- **Hex Mark** (CD 3) — Mark on target.
- **Sap Scales** (CD 3) — Medium hit; Drain 50%.
- **Lullaby Wings** (CD 4) — Slow 3 on target.

**15. Dusklady** — HP 3 · ATK 3 · DEF 2 · SPD 4
- **Doom Whisper** (CD 4) — Doom 3 on target.
- **Silken Seal** (CD 3) — Seal.
- **Nightfall Wings** (CD 3) — Medium hit; Mark and Daze 1.
- **Dark Veil** (CD 3) — Evade; Haste 1.

### Line L4 — Hit-and-run

**16. Gustling** — HP 2 · ATK 3 · DEF 1 · SPD 4
- **Feather Cut** — Light hit; Priority.
- **Wind Hop** (CD 2) — Medium hit; Pivot.
- **Updraft Dive** (CD 3) — Charge: Evade this turn, Heavy hit next turn.
- **Tailwind** (CD 4) [Ally] — Haste 3.

**17. Cyclonix** — HP 3 · ATK 4 · DEF 2 · SPD 4
- **Cyclone Strike** — Medium hit; Pivot.
- **Whirl Away** (CD 3) — Force-switch one enemy.
- **Razor Gale** (CD 2) — Heavy hit; 25% chance of 1.5x damage.
- **Eye Dart** (CD 3) — Evade; Focus on self.

### Line L5 — Support / shield

**18. Glimmerling** — HP 3 · ATK 1 · DEF 2 · SPD 3
- **Sparkle Tap** — Light hit.
- **Prism Shield** (CD 3) [Ally] — Shield 15% max HP.
- **Lucky Glint** (CD 3) [Ally] — Focus; its next move cannot miss.
- **Dazzle** (CD 3) — Daze 2 on target.

**19. Prismaw** — HP 4 · ATK 2 · DEF 3 · SPD 3
- **Refract Beam** (CD 2) — Medium hit; ignores Shield and Armor.
- **Crystal Aegis** (CD 4) [Ally] — Shield 30% max HP; Thorns 2.
- **Spectrum Boost** (CD 4) [Ally] — ATK, DEF and SPD +20% for 3 turns.
- **Mirror Shard** (CD 3) — Reflect the next debuff applied to you back at its source.

### Line L6 — Trapper

**20. Burrowby** — HP 3 · ATK 2 · DEF 3 · SPD 2
- **Claw Scratch** — Light hit; 20% chance of Wound 2.
- **Dig In** (CD 3) — Charge: Evade this turn, Medium hit next turn.
- **Pitfall** (CD 3) — Trap 2.
- **Dirt Cloud** (CD 3) — Daze 1 on all enemies.

**21. Tunnelord** — HP 4 · ATK 3 · DEF 4 · SPD 2
- **Cave-In** (CD 2) — Heavy hit; Trap 1.
- **Spike Pit** (CD 3) — Wound 3 and Trap 2.
- **Rumble** (CD 2) — Spread Medium hit; Slow 1.
- **Fortify Burrow** (CD 4) — Armor 3 and Regen 2 on self.

### Line L7 — Control

**22. Frostwisp** — HP 2 · ATK 2 · DEF 2 · SPD 3
- **Chill Breath** — Light hit; Slow 1.
- **Frost Veil** (CD 3) — Armor 2 on self.
- **Icicle Prick** (CD 3) — Medium hit; 40% chance of Slow 2.
- **Snow Drift** (CD 5) — Stun.

**23. Glacierra** — HP 4 · ATK 3 · DEF 3 · SPD 2
- **Glacial Slam** (CD 2) — Heavy hit; Slow 2.
- **Flash Freeze** (CD 5) — Stun.
- **Permafrost** (CD 4) — Armor 3 on self; attackers get Slow 1.
- **Blizzard Veil** (CD 4) — Daze 2 on all enemies.

### Line L8 — Trickster

**24. Mirrorat** — HP 2 · ATK 2 · DEF 2 · SPD 4
- **Cheeky Bite** — Light hit.
- **Mimic Move** (CD 2) — Use the target's last-used move at 70% power.
- **Decoy** (CD 3) — Shield 15% max HP; Evade.
- **Pilfer** (CD 3) — Steal one buff from the target.

**25. Reflectrix** — HP 3 · ATK 3 · DEF 2 · SPD 4
- **Hall of Mirrors** (CD 4) — Reflect the next damaging move at 100%.
- **Copycat Strike** (CD 2) — Use the target's last-used move at full power.
- **Inversion** (CD 5) — Turn all of the target's buffs into matching debuffs.
- **Trick Step** (CD 3) — Medium hit; Daze 1; Priority.

---

## Rares (single stage)

**26. Chronowl** — HP 3 · ATK 3 · DEF 2 · SPD 4
- **Stitch in Time** (CD 4) — Reset all of your cooldowns.
- **Time Skip** (CD 3) — Priority; your next move also gets Priority.
- **Fateful Hoot** (CD 4) — Doom 2 on target.
- **Hourglass Gaze** (CD 3) — Slow 2 and Mark on target.

**27. Ironmite** — HP 4 · ATK 3 · DEF 5 · SPD 1
- **Rivet Bite** — Medium hit.
- **Plate Up** — Gain a Plate (max 5): each Plate gives -6% damage taken.
- **Rust Cloud** (CD 3) — Target DEF -30% for 3 turns.
- **Cannonball Roll** (CD 3) — Heavy hit; +10% damage per Plate.

**28. Sangrebat** — HP 3 · ATK 3 · DEF 2 · SPD 4
- **Fang Sip** — Medium hit; Drain 50%.
- **Blood Pact** (CD 4) — Lose 20% HP; ATK +50% for 3 turns.
- **Crimson Mist** (CD 3) — Wound 3 on target; you heal 5% max HP each turn it lasts.
- **Night Flit** (CD 3) — Evade; heal 10% HP.

**29. Mimicube** — HP 4 · ATK 3 · DEF 3 · SPD 2
- **Cubic Bump** — Light hit.
- **Disguise** (CD 4) — Copy the target's current stat buffs and debuffs onto yourself.
- **Gelatinous Absorb** (CD 3) — Counter: absorb the hit and heal for 50% of its damage.
- **Shapeshift Slam** (CD 3) — Damage equals 1.2x the last damage the target dealt to you.

**30. Ashenfeather** — HP 3 · ATK 4 · DEF 2 · SPD 3
- **Cinder Wing** — Medium hit.
- **Rebirth Pyre** (Once) — The next time you would faint, revive at 40% HP instead.
- **Ash Veil** (CD 3) — Shield 15% max HP; Cleanse.
- **Solar Dive** (CD 3) — Charge: Heavy hit next turn that ignores Shield.

**31. Kaleidra** — HP 3 · ATK 4 · DEF 2 · SPD 3
- **Chaos Bolt** — Medium hit; random bonus effect (Wound 2, Slow 1, Daze 1, Mark or Focus).
- **Wild Card** (CD 3) — Randomly one of: heal 25% HP, a Heavy hit, Haste 2, or Shield 20% max HP.
- **Prismatic Shatter** (CD 3) — Remove all of the target's buffs; deal +20% damage per buff removed.
- **Anarchy** (CD 5) — Reset every active friend's cooldowns and clear every status on the field, allies and enemies.

---

## Notes and open design questions

- **Archetypes covered:** tank/stall, tempo, burst/setup, wall/counter, healer, curse/debuff, hit-and-run, support, trapper, control, trickster, plus six oddball rares.
- **Anti-snowball:** Stun has a long cooldown and Doom can't KO outright, to avoid "win by lock" patterns.
- **Triples:** support friends (Dewdrip/Tidalisk, Glimmerling/Prismaw) gain a lot of value from [Ally] moves. Spread moves are priced at -25% damage.
- **To check in balance sims:** Evade-heavy kits (Zephox, Dusklady, Cyclonix), Counter loops (Boulderon vs Molten Guard), and Anarchy resetting everything.
- **Names and designs are placeholders.** Easy to rename or swap any friend or move.


---

## Expansion (ids 31 to 47)

Each new friend also lives in one or two battle areas (see below). Move details are shown in the in-game Dex and
Friends tab; this is the overview. The sprites are generated from a per-line motif (mushroom cap, clam shell, gear,
curled horns, ...) so each friend looks like what it does.

| # | Friend | Stage | Role | Moves |
|---|---|---|---|---|
| 32 | Sporeling | 1 of 2 | Fungus control | Cap Bonk, Spore Puff (spread Daze), Rot Touch (Wound 3), Mulch Meal (heal + Regen) |
| 33 | Mycelord | 2 of 2 | Poison support | Toadstool Slam, Choking Cloud (spread Wound), Fungal Network (ally Regen + Shield), Parasite Bloom (Wound + leech) |
| 34 | Sandpup | 1 of 2 | Evasive harasser | Sand Kick, Dust Devil (Evade + Haste), Quicksand Snap (Slow), Sinkhole Ambush (dodging charge) |
| 35 | Dunewyrm | 2 of 2 | Trapper | Sandstorm (spread Daze 2), Mirage Coil (Evade + Armor), Constrict (Trap), Serpent Dance (ally Haste + Focus) |
| 36 | Cogpup | 1 of 2 | Clockwork tempo | Wrench Whack, Oil Slick (Slow foe, Haste self), Tighten Bolts (Armor + Focus), Spring Dash (Priority) |
| 37 | Gearhound | 2 of 2 | Empowered bruiser | Piston Punch (ignores DEF), Overclock (ATK + SPD up), Gear Grind (DEF down), Safety Shell (Shield + Cleanse) |
| 38 | Pearlclam | 1 of 2 | Shell support | Shell Tap, Clam Up (Armor + Shield), Pearl Shot (Daze), Tide Pool (ally heal + Regen) |
| 39 | Pearlarch | 2 of 2 | Thorny wall | Pearl Barrage (3 random hits), Iron Shell (Armor + Thorns), Riptide Slam (force switch), Lustrous Veil (ally Shield + Cleanse) |
| 40 | Cragoat | 1 of 2 | Ram | Head Butt (recoil), Mountain Leap (dodging charge), Horn Lock (Trap), Steady Footing (Armor + Cleanse) |
| 41 | Summitaur | 2 of 2 | Heavy charger | Avalanche Charge, Thunderhoof (spread Daze), Cliffside Stance (strong Counter), Summit Roar (spread Slow + ATK up) |
| 42 | Finlet | 1 of 3 | Tempo support | Flick Splash, Tidal Dart (Priority), Bubble Net (Trap), Schooling (ally Haste + Regen) |
| 43 | Reefray | 2 of 3 | Glider | Stingray Lash (Wound), Glide Away (Evade + Haste), Electro Pulse (spread Daze), Coral Wall (ally Armor + Shield) |
| 44 | Leviathor | 3 of 3 | Sea power | Abyssal Crush (Slow), Maelstrom (spread force-switch), Deep Current (ally Regen + Cleanse), Kraken's Grasp (Trap + Wound) |
| 45 | Eclipsa | Rare | Curse and evasion | Crescent Slash (Mark), Lunar Eclipse (spread Daze + Slow), Dusk Step (Evade + Haste + Priority next), Starlit Hex (Doom + Mark) |
| 46 | Mossback | Rare | Regrowing wall | Shell Bash, Garden Grows (ally Regen + Shield), Root Snare (Trap + Slow), Sunbaked Rest (big heal) |
| 47 | Hailwing | Rare | Spread control | Frost Gust (spread Slow), Hailstorm (3 random hits, chance of Slow), Wind Chill Dive (dodging charge), Jet Stream (ally Haste + Focus) |
| 48 | Magmaroth | Rare | Lava bruiser | Magma Fist (Wound), Molten Carapace (Armor + Thorns), Eruption (spread Wound), Obsidian Rage (self-cost power-up) |

(The # column is the Dex number, starting at 1.)

## Battle areas

| Area | Needs | Level bonus | Coins | Friends found there |
|---|---|---|---|---|
| Sunny Meadow | start | +0 | x1.0 | Mossling, Zapling, Cindrel lines, Dewdrip, Gustling, Burrowby, Sandpup lines |
| Whispering Woods | 3 wins | +1 | x1.2 | Mossling, Gloomoth, Sangrebat, Sporeling lines, Eclipsa, Mossback |
| Echo Caverns | 8 wins | +2 | x1.4 | Pebblet, Glimmerling, Burrowby, Sandpup, Cogpup lines, Ironmite, Sangrebat, Magmaroth |
| Sapphire Shore | 14 wins | +3 | x1.6 | Dewdrip, Mirrorat, Pearlclam, Finlet lines, Mimicube, Mossback |
| Stormcrest Peaks | 22 wins | +4 | x1.8 | Zapling, Pebblet, Gustling, Frostwisp, Cragoat lines, Hailwing, Ashenfeather |
| Ember Ruins | 32 wins | +6 | x2.2 | Cindrel, Gloomoth, Glimmerling, Mirrorat, Cogpup lines, Chronowl, Kaleidra, Eclipsa, Mimicube, Magmaroth |


---

## Expansion II (Dex numbers 49 to 150)

102 more friends: 14 new three-stage lines, 18 new two-stage lines and 24 new rares, each tied to one or more battle
areas. They are generated from `tools/gen_roster.py` (names, concepts, stats, areas and sprite motifs are written by hand
in that file; each role expands into four balanced moves, and rare friends carry hand-written moves). Every one of the
600 move names in the game is unique. In-game move descriptions are in the Dex.

Roles: **swarm** (random multi-hits, spread chip damage), **regenerator** (self and ally regrowth, drain), **controller**
(slows, traps, stuns), **bruiser** (recoil hits and attack boosts), **bomber** (self-damage power-ups, spread blasts),
**tank** (armor, counters, thorns), **trickster** (priority, evasion, steals and copies), **support** (ally heals, buffs,
shields), **cursed** (Mark, Doom, Seal, spread Daze), **charger** (two-turn dodging hits, traps), **counter**
(retaliation).

| # | Friend | Stage | Role | Found in | Moves |
|---|---|---|---|---|---|
| 49 | Bumblet | 1/3 | swarm | Meadow, Woods | Tiny Sting, Buzz Barrage, Pollen Cloud, Wing Hum |
| 50 | Honeybrute | 2/3 | swarm | Meadow, Woods | Stinger Jab, Hive Flurry, Honey Splash, Drone Rally |
| 51 | Queenbuzz | 3/3 | swarm | Meadow, Woods | Royal Stinger, Swarm Volley, Wax Storm, Queen's Decree |
| 52 | Mudpup | 1/3 | regenerator | Woods, Shore | Mud Pie, Wallow, Muddy Paws, Slurp Silt |
| 53 | Bogbark | 2/3 | regenerator | Woods, Shore | Swamp Snap, Peat Blanket, Bog Tonic, Mire Drain |
| 54 | Swampcrown | 3/3 | regenerator | Woods, Shore | Reed Whip, Lotus Bloom, Marsh Blessing, Quagmire Grasp |
| 55 | Flurrcub | 1/3 | controller | Peaks | Snowball, Chilly Nuzzle, Flurry Spray, Drift Pin |
| 56 | Snowmane | 2/3 | controller | Peaks | Frost Nip, Numbing Roar, Whiteout Gust, Deep Freeze |
| 57 | Blizzarion | 3/3 | controller | Peaks | Glacier Mane Slash, Hoarfrost Howl, Polar Cyclone, Absolute Zero |
| 58 | Thornet | 1/3 | bruiser | Meadow, Woods | Briar Scratch, Reckless Thorn Rush, Bloom Rage, Sap Sting |
| 59 | Rosewhip | 2/3 | bruiser | Meadow, Woods | Vine Lash, Barbed Charge, Petal Fury, Nectar Leech |
| 60 | Bramblequeen | 3/3 | bruiser | Meadow, Woods | Queen's Lash, Bramble Stampede, Crimson Bloom, Thorned Embrace |
| 61 | Cometling | 1/3 | bomber | Peaks, Ruins | Star Bonk, Shooting Charge, Stardust Surge, Meteor Shower |
| 62 | Starfall | 2/3 | bomber | Peaks, Ruins | Comet Tail, Falling Star, Supernova Wish, Asteroid Rain |
| 63 | Novadrake | 3/3 | bomber | Peaks, Ruins | Nova Claw, Cosmic Crash, Stellar Overload, Event Horizon |
| 64 | Urchi | 1/3 | tank | Shore | Spine Poke, Curl Tight, Prickle Coat, Tidepool Nap |
| 65 | Spineurchin | 2/3 | tank | Shore | Needle Jab, Sea Spine Guard, Barbed Reprisal, Brine Recovery |
| 66 | Pricklord | 3/3 | tank | Shore | Trident Spike, Spiked Bulwark, Pincushion Retort, Abyssal Rest |
| 67 | Lavapup | 1/3 | bruiser | Caverns, Ruins | Ember Chomp, Lava Lunge, Heat Surge, Scorch Gnaw |
| 68 | Magmahound | 2/3 | bruiser | Caverns, Ruins | Magma Maw, Molten Pounce, Core Rage, Cinder Drain |
| 69 | Calderon | 3/3 | bruiser | Caverns, Ruins | Caldera Claw, Volcanic Tackle, Eruption Heart, Pyroclast Bite |
| 70 | Breezecub | 1/3 | trickster | Meadow, Peaks | Gust Swipe, Air Skip, Dizzy Draft, Shove Breeze |
| 71 | Galelynx | 2/3 | trickster | Meadow, Peaks | Slipstream Slash, Tailwind Blur, Vertigo Gale, Pilfer Wind |
| 72 | Cycloneth | 3/3 | trickster | Meadow, Peaks | Cyclone Claw, Eye Wall Dodge, Whirling Daze, Echoing Gust |
| 73 | Chimey | 1/3 | support | Meadow, Ruins | Ting Strike, Soothing Ring, Quickstep Chime, Clear Tone |
| 74 | Belltoll | 2/3 | support | Meadow, Ruins | Clapper Bonk, Healing Peal, Tempo Toll, Sanctuary Note |
| 75 | Carillon | 3/3 | support | Meadow, Ruins | Resonant Smash, Choir of Mending, Allegro Cascade, Hymn of Warding |
| 76 | Shadowcub | 1/3 | cursed | Woods, Ruins | Shade Pounce, Fading Whisper, Silence Paw, Gloom Mist |
| 77 | Duskpanther | 2/3 | cursed | Woods, Ruins | Dusk Claw, Dread Countdown, Muzzle Shadow, Veil of Night |
| 78 | Nightstalker | 3/3 | cursed | Woods, Ruins | Stalker's Mark, Doom Prowl, Hush of Ruin, Nightfall Shroud |
| 79 | Kelpie | 1/3 | charger | Shore | Wave Nip, Breaker Dive, Foam Shield, Rip Tether |
| 80 | Tidemane | 2/3 | charger | Shore | Surf Kick, Crest Leap, Sea Spray Guard, Undercurrent Bind |
| 81 | Maelstrider | 3/3 | charger | Shore | Gale Hoof, Whirlpool Plunge, Brine Armor, Maelstrom Snare |
| 82 | Caterpix | 1/3 | support | Meadow, Woods | Silk Spit, Leaf Snack, Cozy Thread, Sticky Cocoon |
| 83 | Cocoonix | 2/3 | support | Meadow, Woods | Cocoon Bump, Chrysalis Rest, Silken Blessing, Armored Wrap |
| 84 | Papilion | 3/3 | support | Meadow, Woods | Wing Flutter, Scale Dust Heal, Rainbow Dust, Butterfly Barrier |
| 85 | Pricklet | 1/3 | counter | Meadow, Caverns | Needle Flick, Spiny Stance, Desert Coat, Sting Bloom |
| 86 | Saguarock | 2/3 | counter | Meadow, Caverns | Arm Swing, Cactus Riposte, Dry Fortress, Flower Barb |
| 87 | Cactitan | 3/3 | counter | Meadow, Caverns | Titan Arm, Sunbaked Reprisal, Oasis Bulwark, Thousand Spines |
| 88 | Snaillet | 1/3 | tank | Meadow, Caverns | Slimy Nudge, Withdraw, Slick Shell, Slow Recovery |
| 89 | Stoneshell | 2/3 | tank | Meadow, Caverns | Shell Ram, Stone Retreat, Granite Reprisal, Mineral Rest |
| 90 | Mountaingast | 3/3 | tank | Meadow, Caverns | Boulder Crash, Fortress Withdrawal, Avalanche Reprisal, Mountain's Patience |
| 91 | Dandycub | 1/2 | spreader | Meadow | Fluff Tackle, Seed Scatter, Wish Gust, Breeze Ride |
| 92 | Puffmane | 2/2 | spreader | Meadow | Mane Whip, Pappus Storm, Dandelion Blizzard, Wishful Sprint |
| 93 | Leapkin | 1/2 | trickster | Meadow, Peaks | Hop Tap, Spring Skip, Carrot Trick, Bunny Bounce |
| 94 | Bounderhare | 2/2 | trickster | Meadow, Peaks | Kick Flurry, Moonlit Leap, Lucky Foot Hex, Burrow Swap |
| 95 | Spotter | 1/2 | counter | Meadow, Woods | Dot Nip, Shell Clasp, Lucky Spots, Aphid Snack |
| 96 | Rubybeetle | 2/2 | counter | Meadow, Woods | Ruby Ram, Carapace Riposte, Polished Shell, Garnet Stinger |
| 97 | Cloudlamb | 1/2 | support | Meadow, Peaks | Fleece Bump, Soft Mist, Cumulus Cushion, Rainy Day |
| 98 | Stormram | 2/2 | support | Meadow, Peaks | Thunder Ram, Rain Tonic, Static Fleece, Squall Shelter |
| 99 | Acornet | 1/2 | tank | Woods | Nut Throw, Bark Up, Squirrel Away, Forest Snack |
| 100 | Oakguard | 2/2 | tank | Woods | Branch Swing, Ironwood Skin, Splinter Reprisal, Sapwell |
| 101 | Foxfire | 1/2 | trickster | Woods, Ruins | Cinder Flick, Spirit Step, Will-o-Wisp Lure, Fox Trick |
| 102 | Kitsunar | 2/2 | trickster | Woods, Ruins | Flame Whisker, Nine-Tail Blur, Phantom Fire Daze, Shape Swap |
| 103 | Fireflit | 1/2 | support | Woods | Spark Peck, Glow Mend, Guiding Flicker, Safe Light |
| 104 | Lanternoth | 2/2 | support | Woods | Moth Dust, Warm Lantern Heal, Beacon Focus, Haven Glow |
| 105 | Geodot | 1/2 | bruiser | Caverns | Pebble Punch, Rock Roll, Crystal Rage, Mineral Munch |
| 106 | Geodrake | 2/2 | bruiser | Caverns | Geode Smash, Landquake Dive, Amethyst Fury, Shard Siphon |
| 107 | Glowworm | 1/2 | support | Caverns | Wriggle Tap, Soft Glow Heal, Aim Lantern, Lumen Cover |
| 108 | Lumenwyrm | 2/2 | support | Caverns | Coil Slam, Radiant Mend, Spotlight Focus, Cavern Shelter |
| 109 | Magnetoad | 1/2 | controller | Caverns, Ruins | Static Tongue, Magnetize, Pull Field, Polarity Lock |
| 110 | Ferrofrog | 2/2 | controller | Caverns, Ruins | Iron Tongue, Attraction Surge, Flux Storm, Magnet Prison |
| 111 | Tadwink | 1/2 | regenerator | Shore | Tail Slap, Puddle Hop, Lily Pad Rest, Wriggle Siphon |
| 112 | Rainfrog | 2/2 | regenerator | Shore | Rain Tongue, Downpour Rest, Thunder Croak Bless, Monsoon Siphon |
| 113 | Jellybloom | 1/2 | cursed | Shore | Tentacle Tap, Sting Countdown, Numbing Pulse, Drift Fog |
| 114 | Medusar | 2/2 | cursed | Shore | Venom Lash, Lethargic Doom, Paralytic Hush, Bioluminous Haze |
| 115 | Scuttler | 1/2 | bruiser | Caverns, Shore | Claw Snip, Sidestep Slam, Pincer Pump, Brine Bite |
| 116 | Scarabrute | 2/2 | bruiser | Caverns, Shore | Pincer Crush, Beachhead Charge, Shell Rage, Tideline Drain |
| 117 | Yetilet | 1/2 | bruiser | Peaks | Snow Swat, Avalanche Roll, Chest Thump, Icicle Gnaw |
| 118 | Frostbrute | 2/2 | bruiser | Peaks | Glacier Fist, Whiteout Rush, Yeti Fury, Permafrost Chomp |
| 119 | Eaglet | 1/2 | charger | Peaks | Talon Peck, Swoop Down, Fledgling Flutter, Gale Grip |
| 120 | Thunderhawk | 2/2 | charger | Peaks | Lightning Talon, Stormdive, Static Plumage, Skyhook Snare |
| 121 | Runepup | 1/2 | tank | Ruins | Glyph Bump, Rune Plate, Etched Retort, Sigil Heal |
| 122 | Runewarden | 2/2 | tank | Ruins | Glyph Smash, Warding Script, Runic Reprisal, Sealed Recovery |
| 123 | Wrapling | 1/2 | cursed | Ruins | Bandage Whip, Curse Wrap, Silent Tomb, Dust Cloud Hex |
| 124 | Pharoth | 2/2 | cursed | Ruins | Tomb Strike, Pharaoh's Countdown, Seal of Ages, Sandstorm Curse |
| 125 | Wisplet | 1/2 | trickster | Woods, Ruins | Boo Poke, Phase Blink, Chill Touch, Haunt Shuffle |
| 126 | Spectrael | 2/2 | trickster | Woods, Ruins | Ghost Claw, Ethereal Veil, Haunting Chill, Spirit Echo |
| 127 | Tessellate | 1/1 | rare warder | Caverns, Ruins | Prism Shard, Mosaic Ward, Kaleidoscope Beam, Fractal Cleanse |
| 128 | Paradoxel | 1/1 | rare time | Ruins | Echo Strike, Rewind, Temporal Mirror, Paradox Swap |
| 129 | Nimbusaur | 1/1 | rare storm | Meadow, Peaks | Thunderclap Stomp, Cloud Cover, Storm Surge, Downpour |
| 130 | Rustwraith | 1/1 | rare rust | Caverns, Ruins | Corroding Touch, Spectral Plate, Oxidize, Flake Reprisal |
| 131 | Voidmaw | 1/1 | rare void | Woods, Ruins | Hungry Bite, Devour Buff, Void Shatter, Singularity |
| 132 | Dawnbringer | 1/1 | rare dawn | Meadow, Peaks | Sunbeam, Daybreak Mend, Golden Hour, Solar Flare |
| 133 | Gloamreaper | 1/1 | rare reaper | Woods, Ruins | Harvest Swing, Soul Tally, Grim Dash, Final Toll |
| 134 | Abyssopod | 1/1 | rare lure | Shore | Lure Glow, Gulp, Pressure Trap, Deep Camouflage |
| 135 | Pangolem | 1/1 | rare plates | Caverns, Ruins | Scale Slam, Roll Up, Plate Stack, Ricochet Roll |
| 136 | Zephyra | 1/1 | rare wind | Meadow, Peaks | Cutting Wind, Updraft Blessing, Zephyr Veil, Gale Force |
| 137 | Cinderqueen | 1/1 | rare fire | Ruins | Ember Scepter, Ashfall, Royal Blaze, Crown of Cinders |
| 138 | Fennecho | 1/1 | rare echo | Meadow, Caverns | Sandy Nip, Echo Bark, Big Ear Hush, Burrow Dodge |
| 139 | Bastionwyrm | 1/1 | rare fort | Caverns, Ruins | Rampart Bite, Fortress Roar, Stone Scales, Siege Breath |
| 140 | Lullabyss | 1/1 | rare sleep | Woods, Shore | Soft Note, Sleep Song, Lullaby Tide, Dreamy Dive |
| 141 | Harlequill | 1/1 | rare chaos | Meadow, Ruins | Pratfall, Juggling Act, Joker's Gambit, Mime Show |
| 142 | Thornmother | 1/1 | rare garden | Meadow, Woods | Briar Smack, Nursery Bloom, Thorn Garden, Lullaby Roots |
| 143 | Prismoth | 1/1 | rare prism | Woods, Ruins | Rainbow Powder, Spectrum Wing, Prism Veil, Glimmer Scales |
| 144 | Epochite | 1/1 | rare time | Ruins | Chrono Nick, Time Pocket, Slow Time, Frozen Instant |
| 145 | Magnaclaw | 1/1 | rare magnet | Caverns, Shore | Pincer Pull, Magnetic Yank, Iron Grip, Field Polarize |
| 146 | Rimewyrm | 1/1 | rare frost | Peaks | Rime Fang, Flash Frostbreath, Deepfreeze Stare, Crystalline Coil |
| 147 | Mirage | 1/1 | rare mirage | Meadow, Caverns, Ruins | Heat Haze, Vanish, Phantom Oasis, Dust Devil Veil |
| 148 | Marrowmancer | 1/1 | rare bone | Woods, Ruins | Bone Spear, Bone Wall, Rattle Hex, Skeletal Grasp |
| 149 | Lumenwing | 1/1 | rare light | Peaks, Ruins | Halo Strike, Radiant Rebirth, Wings of Warding, Dawn Beam |
| 150 | Gambit | 1/1 | rare chess | Ruins | Knight's Hop, Sacrifice Play, Fork Strike, Castling |
