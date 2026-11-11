# Goal Friends — Design Doc (draft v0.2)

_Status: design iteration only. Personal use, Windows laptop. Prototype comes when requested._

A terminal, turn-based creature-collector where **real-life tasks are the only way to progress**.
You can't grind in-game. You level up, catch, and evolve creatures by getting things done offline,
then spend that progress in Pokémon-style battles.

## 1. Pillars

1. **Real effort drives everything.** No in-game grinding loop that bypasses tasks.
2. **Battles are the payoff, not the progress source.** Winning feels good because you earned the team.
3. **Gentle on misses.** Skipping a day makes creatures sleepy, never dead. Guilt is a bad motivator.
4. **Fast first reward.** A first creature and first level-up within 5 minutes of install.

## 2. Core loop

```
 add task -> do it IRL -> mark done -> earn XP + Energy + Coins
                                  |
        +-------------------------+----------------------------+
        v                         v                            v
  level up/evolve          spend Energy on battles        spend Coins on
  assigned creature        (stronger foes, bosses)        Capsules (new creatures)
```

## 3. Tasks

| Field | Notes |
|---|---|
| `title` | Free text |
| `category` | `fitness`, `study`, `chores`, `creative`, `social`, `health` |
| `effort` | `S`, `M`, `L`, `XL` (self-rated, maps to base XP: 10 / 25 / 60 / 120) |
| `repeat` | none, daily, weekly (recurring tasks reset automatically) |
| `assigned_to` | Optional creature that receives the XP; default is the active party lead |

**Anti-cheat (honor system, light friction):**
- Daily XP cap (e.g. 400) with diminishing returns after the first 3 completions of the same task.
- Completing a task asks for a one-line confirm ("Really done? y/n").
- No penalties for being honest about small tasks. S-effort tasks are fine, they just give little.

**Rewards per completion:**
- `XP = base(effort) * streak_multiplier * category_affinity`
- `Energy = effort tier (S=1, M=2, L=4, XL=8)`, spent on battles
- `Coins = base(effort) / 5`
- Streak multiplier: +5% per consecutive active day, capped at +50%.
- Category training: each task category also nudges specific stats (see section 4). Types are NOT tied to categories.

## 4. Creatures

Each creature has:
- **Type** (elemental, independent of task categories)
- **Base stats:** HP, Attack, Defense, Speed
- **Favorite category** (flavor trait, optional small XP bonus, see below)
- **Level** (1-50), derived from XP
- **Moves:** 4 slots, learned at level thresholds
- **Evolution:** 2 or 3 stages, at levels ~16 and ~32

**Types are elemental and separate from tasks.** Proposed 8 types (original names, Pokémon-like feel):
Flame, Tide, Leaf, Volt, Stone, Gale, Shade, Lumen. The type chart is a small, readable web
(each type strong against 2, weak to 2), finalized during balancing.

**How tasks affect creatures, indirectly:**
- **Category training:** each task category adds small permanent stat gains to the creature that
  receives the XP, like training in the games. This is where "do real things, get stronger" shows up
  without locking you to a type.

  | Task category | Trains |
  |---|---|
  | fitness | Attack, HP |
  | study | Speed, special power |
  | chores | Defense |
  | health | HP, regen |
  | creative | Move variety / crit chance |
  | social | Support (heal and buff strength) |

- **Favorite category:** each creature has one favorite (e.g. a Stone creature that loves "chores").
  Tasks in that category give it +25% XP. It is a flavor and team-building hook, not a hard rule,
  and it's decoupled from type, so any type can love any category.
- This leaves open a later option: unlock moves or evolutions by a category-specific task count.

**Roster (31 at launch, original designs inspired by Pokémon):**
- 3 starter lines x 3 stages = 9
- 8 two-stage lines = 16
- 6 single-stage creatures (rare/special) = 6
- Total: 31. Names, types, and stats are TBD in a roster pass.

**XP curve:** `xp_to_next(level) = 20 * level^1.6` — fast early levels, slower later.

**Getting new creatures:**
- **Capsules** (bought with Coins) hatch a random creature, rarity weighted.
- **Milestone gifts:** 7-day streak, 100 tasks, first L-task in each category, etc., each guarantee a specific creature.
- **Pokédex:** shows silhouettes of undiscovered creatures, so you can see what's left.
- Duplicates convert to Coins or a small XP boost.

## 5. Battles (turn-based)

Battles cost **Energy**, which only comes from completed tasks. No Energy, no battling.
This is the key link: real life feeds the fun part.

**Format:** 1v1 (v1), expand to 3v3 teams later.

**Turn order:** by Speed (ties random).

**Actions per turn:** Fight (pick a move), Switch (3v3 only), Item (consumables from Capsule drops), Run.

**Damage (v1 formula):**
```
damage = ((2*level/5 + 2) * power * atk / def / 50 + 2) * type_mult * random(0.85, 1.0)
```

**Battle modes:**
- **Wild encounters:** cost 1 Energy, random foe near your level, chance to **catch** it (extra route to new creatures).
- **Trainer duels:** cost 2 Energy, fixed AI teams, reward Coins and items.
- **Boss battles:** unlocked by streak milestones, cost 5 Energy, guaranteed rare reward.

**AI:** v1 picks the highest expected-damage move with some randomness. Bosses get scripted patterns.

**Sleepy status (missed days):** if you skip a day, party members start battles at -10% Speed
and Attack, clearing after your next completed task. No loss of levels or creatures.

## 6. Progression and pacing

| Moment | Target |
|---|---|
| First creature | immediately on first launch (starter pick) |
| First level-up | first S/M task completed |
| First new creature | within day 1 (starter Capsule gift) |
| First evolution | ~1 week of regular use |
| Full dex | months, by design |

## 7. Tech approach (terminal)

- **Scope:** personal use, single user, runs on a Windows laptop. No accounts, no network.
- **Language:** Python 3. Simple to write, easy to test.
- **UI:** `Textual` or `rich` for a menu-driven TUI (both work well in Windows Terminal).
  Python's built-in `curses` doesn't ship on Windows, so it's not an option without extra packages.
- **Storage:** single JSON save file in `%APPDATA%\GoalFriends\save.json` (versioned schema), easy to back up.
- **Content as data:** creatures, moves, and the type chart live in JSON/YAML files, separate from code,
  so balancing doesn't need code changes.
- **Structure (proposed):**
  ```
  goal_friends/
    models.py      # Task, Creature, Move, Save
    rewards.py     # XP / coin / energy math
    battle.py      # battle engine (pure logic, no UI)
    ai.py          # opponent move selection
    storage.py     # load/save
    ui/            # screens: tasks, party, dex, battle, shop
    data/          # creatures.json, moves.json, types.json
  tests/
  ```
- Keep `battle.py` and `rewards.py` UI-free so they're unit-testable and balanceable via simulation.

## 8. Roadmap

1. **M1 — Task engine:** add/complete tasks, XP, levels, streaks, save file. (CLI only)
2. **M2 — Creatures and dex:** starter pick, ~12 creatures, Capsules, party screen.
3. **M3 — Battles:** 1v1 wild encounters, damage formula, simple AI, Energy spending.
4. **M4 — Depth:** moves and learning, evolutions, trainer duels, catching, 3v3.
5. **M5 — Polish:** bosses, milestone gifts, balancing sims, TUI styling.

## 9. Open questions

- Roster pass: names, types, base stats, and evolution lines for the 31 creatures.
- Type chart: finalize the 8-type web.
- Should Energy cap or carry over indefinitely? (suggest cap at 20 so it doesn't hoard)
- Are recurring tasks always manual to complete, or reminded?
- Prototype scope (to decide when you say go): likely tasks + XP + 1v1 battles with a handful of creatures.
