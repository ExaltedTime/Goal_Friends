# Goal Friends — Roster Draft (v0.2)

31 friends, original designs inspired by Pokémon. **No types.** Each has 4 unique moves (no move name is
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
