# Goal Friends — Design Doc (draft v0.1)

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
- Category affinity: x1.5 XP if the receiving creature's type matches the task category.

## 4. Creatures

Each creature has:
- **Type** (tied to a task category, see below)
- **Base stats:** HP, Attack, Defense, Speed
- **Level** (1-50), derived from XP
- **Moves:** 4 slots, learned at level thresholds
- **Evolution:** 2 or 3 stages, at levels ~16 and ~32

| Task category | Creature type | Flavor |
|---|---|---|
| fitness | Blaze | high Attack/Speed |
| study | Mind | special moves, status effects |
| chores | Earth | high Defense/HP |
| creative | Spark | wild-card, random effects |
| social | Tide | healing and team support |
| health | Leaf | regen, endurance |

**Type chart:** a simple cycle plus neutral, kept small for readability, e.g.
Blaze > Leaf > Earth > Tide > Blaze, Mind and Spark trade advantage. Finalize during balancing.

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

- **Language:** Python 3. Simple to write, easy to test.
- **UI:** `Textual` or `rich` for a menu-driven TUI. Plain `curses` is the dependency-free fallback.
- **Storage:** single JSON save file in `~/.goal_friends/save.json` (versioned schema), so it's easy to back up.
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

- How many creatures at launch? (suggest 12 base lines, ~30 with evolutions)
- Should Energy cap or carry over indefinitely? (suggest cap at 20 so it doesn't hoard)
- Do recurring tasks auto-complete-able via reminders, or always manual?
- Any desire for sharing/leaderboards later (Goal "Friends")? Would change storage design.
