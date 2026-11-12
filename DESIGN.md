# Goal Friends — Design Doc (draft v0.9)

_Status: working prototype built (see section 11). Personal use, Windows laptop, native C++.
Companion file: [ROSTER.md](ROSTER.md) (150 creatures and their moves)._

A creature-collector that lives as a slim strip along the bottom of your screen. **Real-life tasks are the
only way to progress.** You can't grind in-game: tasks give you XP, Energy and Coins, and you spend
them on levelling your friends, hatching new ones, and battling.

## 1. Pillars

1. **Real effort drives everything.** No in-game grinding loop that bypasses tasks.
2. **Battles are the payoff, not the progress source.** Winning feels good because you earned the team.
3. **Strategy over stats.** With no type chart, depth comes from each friend's four unique moves.
4. **Gentle on misses.** Skipping days makes friends sleepy, never dead. Guilt is a bad motivator.
5. **Fast first reward.** A first friend and first level-up within minutes of first launch.

## 2. Core loop

```
 add task -> do it IRL -> mark done -> earn XP + Energy + Coins
                                  |
        +-------------------------+----------------------------+
        v                         v                            v
  level up / evolve        spend Energy on battles       spend Coins on Eggs
  the chosen friend        (wild, duels, bosses)         and Catch Charms
```

## 3. Tasks

### 3.1 Task fields

| Field | Notes |
|---|---|
| `title` | Free text |
| `effort` | `S`, `M`, `L`, `XL` (self-rated; base XP 10 / 25 / 60 / 120) |
| `schedule` | one-off, or a recurrence rule (see 3.2) |
| `category` | Optional label for filtering and stats only. **No gameplay effect.** |
| `assigned_to` | Friend who receives the XP (default: the party lead) |
| `remind` | Reminder settings (see 3.3) |

### 3.2 Recurring tasks

A recurring task is a **rule** that generates one **occurrence** per due date. Occurrences show in a
"Today" list. Completing one marks that occurrence done.

Recurrence rules:
- Daily
- Specific weekdays (e.g. Mon/Wed/Fri)
- Every N days (e.g. every 3 days)
- Weekly (on a given day)
- Monthly (on a given date, or "last day")

Optional timing on each rule:
- A **target time** (e.g. 07:30) or a **window** (morning / afternoon / evening / any time that day).
- A **grace period** after the window closes. Late completions during grace still count, at 50% XP.

Rewards and streaks:
- Each recurring task has its own **streak** (consecutive occurrences done). It gives +5% XP per step, capped at +50%.
- There is also a **global streak**: consecutive days with at least one completion. It gives a similar small bonus.
- Missing an occurrence resets that task's streak. Rules can be paused (e.g. vacation) without breaking the streak.

### 3.3 Reminders

- Delivered as Windows tray notifications (balloon tips via `Shell_NotifyIcon`), so the app needs to be running (it lives in the tray).
- **Initial reminder** at the task's target time (or at the start of its window).
- **Nudges** if still not done: default +30 min, then +2 h, then once near the end of the window. Configurable per task.
- **Snooze** from the notification or the panel: 15 min / 1 h / until tomorrow.
- **Quiet hours** (default 22:00 to 08:00): reminders are held until quiet hours end.
- Friends can "nag" in the world too: a speech bubble appears over a creature when a task is due.
- If the laptop was asleep or the app was closed, missed reminders collapse into a single catch-up notification.

### 3.4 Rewards per completion

- `XP = base(effort) * (1 + streak_bonus)`, halved if completed late (grace period)
- `Energy`: S=1, M=2, L=4, XL=8. Spent on battles. **No cap**: unspent Energy accumulates.
- **Focus sessions** (timer: 15/25/45 min) also award Energy (1 per 10 min) and XP (1 per minute) to the lead, count as activity for the streak, and **open the battle screen when they finish**.
- `Coins = base(effort) / 5`
- No daily XP cap (personal use). An optional "really done?" confirm is a setting, on by default, and easy to switch off.

## 4. Creatures ("Friends")

- No types and no type chart. Strategy lives in moves (see section 5).
- Each friend has: **base stats** (HP, ATK, DEF, SPD), **level** (1 to 50) and **exactly 4 unique moves**.
- Moves are unique to each creature, including between evolution stages (an evolved form has its own 4 moves).
- **Evolution:** 3-stage lines at levels 16 and 32, 2-stage lines at level 20. Six single-stage rares.
- **Roster:** 150 friends, listed in [ROSTER.md](ROSTER.md): 3 starter lines, 31 two-stage lines, 18 three-stage lines in total and 34 rares.
- **Stat growth:** `stat = base * (1 + 0.06 * (level - 1))` plus a small flat bonus (tuned in balancing).

**Getting friends:**
- **Starter:** pick one of three at first launch.
- **Eggs:** bought with Coins. An egg hatches after you complete a set number of tasks (e.g. 5), so hatching is tied to real effort.
- **Catching:** wild encounters can be befriended with a Catch Charm (higher chance at low HP).
- **Milestone gifts:** streaks and task-count milestones guarantee specific friends.
- **Party and Box:** you bring a **party of 6** into battle. Every other friend you own lives in the Box and can be swapped in between battles.
- **Dex:** shows silhouettes of friends not yet found. Duplicates convert to Coins.

> Note: an earlier draft had a "favorite category" per creature (bonus XP for matching tasks).
> It's been dropped. Categories are now only labels.

## 5. Battles (turn-based, strategy-driven)

Battles cost **Energy**, which only comes from completed tasks. This is the link between real life and the fun part.

**Format: triple battles.** Each side brings a **team of 6** and has **3 friends active at once**, with
3 on the bench. A side loses when all 6 of its friends have fainted. Your team is fully healed after each battle.

**Each turn:** every active friend picks an action (up to 3 per side), all chosen at once. The 6 actions then
resolve in order of priority, then SPD (ties random).
- **Fight:** use one of the friend's 4 moves, and pick a target where the move needs one
- **Switch:** swap that friend with a bench friend (resolves before moves)
- **Run:** wild battles only

**Targeting:** there are no board positions or adjacency rules. Every enemy is always targetable.
- Default moves hit **one chosen enemy**.
- **Spread** moves hit all enemies, at 75% damage (status effects apply in full).
- **[Ally]** moves can be aimed at any active ally, including the user (heals, shields, buffs, cleanses).
- Moves with several hits say whether they hit one target or random targets.
- Buffs and debuffs belong to the individual friend. Statuses and cooldowns reset when a friend leaves the field,
  except cooldowns, which keep ticking on the bench.

**Scuffle:** if every move is on cooldown, a friend uses a basic Light hit so it is never stuck (several evolved friends have no cooldown-free move).

**Fatigue:** after turn 30, everyone loses an increasing amount of max HP each turn, so stalemates always end.

**When a friend faints:** it's removed and a bench friend of your choice takes the slot at the end of the turn.
If the bench is empty, that slot stays empty and you fight on with fewer.

**Moves:** each move has a power tier, a cooldown, and often an extra effect.
- Power tiers: **Light** (~40), **Medium** (~70), **Heavy** (~110, usually cooldown 2+), **Utility** (no damage).
- **Cooldowns** replace PP. A move on cooldown can't be used. Cooldowns tick down at the end of each turn.
- Basic Light moves have no cooldown, so you are never stuck without an action.
- Many moves are Utility or hybrid: setup, defence, disruption, traps, delayed hits.

**Damage:**
```
damage = ((2*level/5 + 2) * power * atk / def / 50 + 2) * random(0.9, 1.0) * modifiers
```

**Status and effect vocabulary (used in ROSTER.md):**

| Effect | Meaning |
|---|---|
| Wound N | Lose 6% max HP at end of each turn for N turns |
| Daze N | The target's attacks have a 30% chance to miss for N turns |
| Slow N / Haste N | SPD -30% / +40% for N turns |
| Armor N | Take 30% less damage for N turns |
| Shield X | Absorbs the next X damage |
| Focus | Next attack deals +50% damage |
| Mark | Next hit received deals +50% damage |
| Regen N | Heal 8% max HP at end of each turn for N turns |
| Thorns N | Attackers take 25% of the damage they deal, for N turns |
| Counter | If hit this turn, retaliate (damage as stated); otherwise wasted |
| Evade | Avoid the next incoming attack this turn |
| Priority | Moves before normal-priority actions |
| Charge | Move takes two turns. The hit lands next turn; during the wind-up you are vulnerable unless stated |
| Drain X% | Heal for X% of damage dealt |
| Seal | Target's last-used move is unusable for 2 turns |
| Trap N | Target cannot be switched out for N turns |
| Stun | Target skips its next action |
| Taunt N | Target can only use damaging moves for N turns |
| Doom N | After N turns, target takes 40% max HP damage (survivable, cannot KO outright) |
| Pivot | After hitting, the user swaps with the healthiest bench friend (chosen automatically in the prototype) |
| Force-switch | Target is swapped with a random bench friend (no effect if its bench is empty) |
| Cleanse | Remove all debuffs from the user |
| Once | Usable once per battle |

Stacking rule: reapplying a status refreshes its duration. It does not stack, except where stated.

**Battle modes:**
- **Wild encounter:** 1 Energy. 1 to 3 wild friends near your level against your 3 active friends. A Catch Charm can be used on a wild friend.
- **Trainer duel:** 2 Energy. 6 vs 6 against a scripted team. Rewards Coins and Eggs.
- **Boss battle:** 5 Energy, unlocked by streak milestones. Special scripted AI. Guaranteed rare reward.

**Pacing target:** about 8 to 15 turns for a duel. HP and damage are tuned so triples don't drag, and balancing sims will check this.

**AI:** v1 scores moves by expected damage, status value and cooldown state, with some randomness.
Bosses follow scripted patterns that you can learn.

**Sleepy status (missed days):** if you complete no tasks in a day, your friends start battles with
**-20% ATK and SPD** until you complete your next task. No levels or friends are ever lost.

## 6. Progression and pacing

| Moment | Target |
|---|---|
| First friend | immediately on first launch (starter pick) |
| First level-up | first S/M task completed |
| First egg | within day 1 (starter gift) |
| First evolution | ~1 week of regular use |
| Full dex | months, by design |

## 7. Tech approach (native C++, no frameworks)

**Decision:** a fully native Windows app in C++ using only the Win32 API and Microsoft's built-in
graphics stack. No SDL, Qt, or UI libraries. Chosen for a tiny footprint and full control over the
window, since it sits on screen all day.

- **Scope:** personal use, single user, Windows laptop. No accounts, no network.
- **Language/toolchain:** C++20, CMake, MSVC (Visual Studio 2022, "Desktop development with C++" workload).
- **Window:** a layered, borderless, always-on-top, full-width window along the bottom edge
  (`WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW`) with per-pixel alpha. Transparent pixels are
  click-through, so it doesn't block other apps. Clicking a creature or UI opens panels.
  Tray icon (`Shell_NotifyIcon`) for show/hide/quit and notifications.
- **Rendering:** Direct2D + DirectWrite (text) with WIC image loading. Fixed-timestep loop, throttled to
  low FPS when idle to save battery.
- **UI:** custom-drawn widgets (buttons, lists, text input, health bars) on Direct2D. This is the
  biggest piece of work, so we keep a deliberately small widget set.
- **Storage:** JSON save in `%APPDATA%\GoalFriends\save.json` (atomic write via temp file + rename). A small
  hand-written JSON reader/writer (`src/core/json.cpp`) is used, so there is no third-party code at all.
- **Content as data:** creatures and moves are declarative tables in `src/core/data.cpp` (effects are plain data
  interpreted by the battle engine). Moving them to external JSON files is a possible later step.
- **Art:** generated rough pixel-art placeholder sprites (PNG sheets, 32x32 or 48x48 per frame, a few idle and walk frames). Swapping in real art later is replacing
  image files and adjusting a frame table.
- **Structure (proposed):**
  ```
  src/
    core/        # platform-free logic: tasks, recurrence, rewards, creatures, battle, ai, save
    platform/    # Win32 window, tray, notifications, input, timing
    render/      # Direct2D renderer, sprites, text
    ui/          # widgets and screens: world, tasks, party, dex, shop, battle
  data/          # creatures.json, moves.json
  assets/        # placeholder sprites
  tests/         # core logic tests (build and run on any OS)
  ```
- **Testing strategy:** `core/` has no Windows or graphics dependencies, so it builds and runs on Linux.
  That gives unit tests for recurrence and rewards, plus simulated battles for balancing, without a
  Windows machine. Anything in `platform/` and `render/` is verified on your laptop.
- **Known risks:** layered-window alpha plus click-through hit testing, Direct2D device loss, custom
  text input (caret, IME, clipboard), and a data-driven battle effect engine (many unique moves).
  We validate the window first with a tiny spike before building on it.

## 8. Desktop layout: the "small world"

- **Full-width strip** (about 120 to 200 px tall) along the bottom edge of the screen.
- A ground line with a simple background. Day/night follows the real clock.
- Party friends wander, idle, nap (when sleepy), and react when you complete a task.
- Click the world (or a friend) to open a compact panel for tasks, party, dex, shop and battles.
  Click away to collapse it.
- Completing a task triggers a small celebration animation and an XP popup.
- Due recurring tasks show a speech bubble over a friend.

## 9. Prototype scope and roadmap

**The prototype includes all the major functions:**
- Full-width transparent bottom strip, wandering friends, tray icon
- One-off and recurring tasks, with reminders, streaks and snooze
- XP, levels, evolution, Energy, Coins, sleepy status
- Shop (Eggs and Catch Charms), egg hatching via tasks, party and dex screens
- All 31 friends with their 4 unique moves, data-driven
- Battles: triples (6-friend teams, 3 active at once) with switching, cooldowns and status effects; wild encounters, trainer duels and one boss
- JSON save and load
- Rough pixel-art placeholders (generated), no sound

**Order of work:**
1. **M0 — Window spike:** transparent, always-on-top, click-through bottom strip with one animated placeholder sprite and a tray icon. You validate it on your laptop first.
2. **M1 — Core logic:** tasks, recurrence, rewards, streaks, save file, with unit tests.
3. **M2 — Battle engine:** data-driven moves and effects, AI, simulated battles for balancing, with tests.
4. **M3 — World and UI:** strip world, task panel, party, dex, battle screens.
5. **M4 — Reminders, eggs, shop, catching, boss:** wire up the remaining systems. This completes the prototype.
6. **M5 — Polish:** balancing sims, real art, sound (later).

## 10. Open questions

- Roster review: react to the 31 friends and their moves in ROSTER.md. Several moves were adjusted for triples (Spread, [Ally]).
- Triple-battle UI: three friends per side plus targeting is a lot of screen for a 120 to 200 px strip, so battles
  likely open in a larger popup panel. Decide the exact layout at prototype time.
- Whether to add a "really done?" confirm by default (currently on, switchable).
- Wild-encounter difficulty scaling and how many wild friends appear.

## 11. Prototype status (v0.6)

Everything in section 9's prototype scope is implemented in one pass. The logic is covered by `tests/test_main.cpp`
(339 checks plus 1500 simulated battles), and the Windows build was cross-compiled and smoke-tested under Wine.

**Verified in automated tests:** recurrence rules, streaks, sleepy, late completion, reminders (stages, snooze, quiet
hours), eggs, evolution, shop, party, save/load, battle engine (all 124 moves used in simulations, no stalls,
sanitizer-clean), sprite generation.

**Verified by driving the real app under Wine:** starter pick, typing a task, completing it with the confirm step,
rewards and egg progress, JSON save, and a full wild battle through the battle UI.

**Not yet verified on real Windows:** text rendering (Wine's Direct2D draws no text, so labels could not be checked),
per-pixel alpha and click-through on the desktop, tray icon and balloon notifications, DPI scaling.

**Known gaps and simplifications:**
- Pivot picks the replacement automatically.
- No sound, no animation beyond walking, bobbing, hops and a day/night ground tint.
- Reminders need the app running (tray). Missed reminders collapse into one balloon.
- Boss unlock is a 3-day streak; reward is a random unowned rare.
- Balance is rough: simulated AI-vs-AI win rates range from about 47% to 73% per friend, average battle about 13 turns.

## 12. Changes in v0.7 (from your prototype feedback)

- **Combat is no longer instant.** The engine still resolves a turn at once, but it records an event and a snapshot of
  the battlefield after every log line. The UI replays them one at a time: the acting friend glows and lunges,
  health bars slide, red and green floating numbers show damage and healing, the log highlights the current line,
  and Skip / Speed buttons let you jump ahead.
- **Guide tab:** how tasks, friends, focus sessions, battles, status effects and areas work.
- **Battle areas:** six areas with their own friends, level bonus, coin multiplier and unlock (battles won).
- **New tasks don't repeat by default.**
- **Roster expanded to 48** (17 new friends with 68 new unique moves).
- **Sprites are tied to abilities:** 24x24 pixel art with a motif per line (sprout and thorns for the regrowing tank,
  lightning tail for the speed fox, flame crest for the burst hound, clock face on the owl, gear on the clockwork dog,
  shell and pearl on the clam, curled horns on the goat, and so on). Evolutions grow visibly.
- **Ground:** more translucent and detailed (layered soil, grass tufts, flowers, bushes, pebbles, fireflies at night).
- **Hotkey Ctrl+Shift+Ô** hides and shows the window. Hiding draws a fully transparent, click-through frame, so the
  hotkey, reminders and timers keep working. If the layout has no Ô key the hotkey falls back to the Slovak position.
- **Focus sessions:** no popup at the end (a notification, and the Battle button pulses). While a session runs, every
  other tab and strip button is locked.
- Wild encounters and bosses scale with your team size, so a lone early friend isn't swamped.

## 13. Ground (v0.8)

The strip's ground is now an all-grass pixel meadow drawn on the same pixel grid as the sprites (one cell = one sprite
pixel = 2 logical px, snapped to whole device pixels so it stays crisp at 125% and 150% scaling):
- three flat grass layers that get darker and more see-through towards the bottom, with a checker-dithered seam
- a repeating 8-column blade pattern on the top edge, tiny tufts laid out in offset (brick-like) rows, and sparse flowers by day. `kTuftStyle` in `drawGround` switches between five textures
- a palette that blends smoothly through dawn, day, noon, dusk and night (no soil, no stones)
- friends are drawn at the same pixel size as the ground, with an oval shadow on the grid

## 14. Roster of 150 (v0.9)

- 102 new friends (Dex 49 to 150), 408 new unique moves (600 in total), all assigned to battle areas.
- They come from `tools/gen_roster.py`: creative content (names, concepts, stats, areas, sprite parts, rare move sets) is
  written by hand there; roles (swarm, tank, support, controller, ...) expand into balanced four-move kits that scale by
  evolution stage. The generator checks that no move name repeats.
- Sprites use a parts system (`sprite_gen.inc`): a body shape per evolution stage plus themed parts (horns, wings, tails,
  crests, shells, lanterns, scythes, ...), so each new line looks like what it does.
- Balance check over 6000 simulated battles: mean 50%, standard deviation about 10 points, strongest friend at 76%. Win
  rate rises with evolution stage, as intended.
- The Dex scrolls.
