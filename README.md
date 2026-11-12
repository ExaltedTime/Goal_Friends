# Goal Friends

A creature collector that lives as a slim strip along the bottom of your screen. **Real-life tasks are the only
way to progress**: finishing tasks (and focus sessions) earns XP, Energy and Coins, which you spend on levelling
friends, hatching eggs, and strategic triple battles.

See [DESIGN.md](DESIGN.md) for the design and [ROSTER.md](ROSTER.md) for all 31 friends and their moves.

## Status: working prototype

Native C++20, Win32 + Direct2D, no third-party libraries. Placeholder art is generated in code.

What's in it:
- Transparent, always-on-top, full-width strip along the bottom of the work area (click-through where empty), tray icon
- One-off and recurring tasks (daily, chosen weekdays, every N days, weekly, monthly), time windows, grace period,
  per-task streaks, reminders as tray notifications with nudges, snooze and quiet hours (22:00 to 08:00)
- XP, levels (1 to 50), evolution, Energy, Coins, global streak, "sleepy" friends after a missed day
- Focus sessions (15 / 25 / 45 min, plus a 1-minute test) that lock the rest of the app while they run and award Energy
  when they finish (no popup: the Battle button pulses)
- Shop (Eggs hatch after 5 completed tasks; Catch Charms), party of 6 plus a Box, dex of 31 friends
- 150 friends with 600 unique moves and ability-themed pixel art
- Triple battles: 6-friend teams, 3 active at once, cooldowns and 20+ status effects, wild encounters, trainer
  duels and a streak-unlocked boss, AI opponents. Turns replay step by step with health bars, floating damage numbers
  and a log (Skip / Speed buttons)
- Six battle areas with their own friends, level bonus and coin multiplier, unlocked by winning battles
- A Guide tab explaining everything in the app
- JSON save in `%APPDATA%\GoalFriends\save.json`

## Build with Visual Studio 2022

1. Install the **Desktop development with C++** workload (includes MSVC, CMake and the Windows SDK).
2. **File > Open > Folder...** and pick this repo. Visual Studio detects `CMakeLists.txt` and configures it.
3. In the toolbar choose the startup item **GoalFriends.exe** and press **F5** (or Ctrl+F5 to run without debugging).
4. To run the logic tests, choose **gf_tests.exe** as the startup item (all checks should pass).

Command line (Developer PowerShell):

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
build\Release\GoalFriends.exe
```

Quit from the tray icon (right-click > Quit). **Ctrl+Shift+Ô** (or the tray icon) hides and shows the window;
reminders keep working while it is hidden.

## Tests

`src/core` has no Windows dependency, so the rules and battle engine are tested on any OS:

```bash
cmake -S . -B build -G Ninja && cmake --build build && ./build/gf_tests
```

That runs unit tests (recurrence, streaks, rewards, reminders, saving) and 1500 simulated AI-vs-AI battles across
the whole roster to catch crashes and degenerate loops.

## Performance

The strip is meant to sit on screen all day, so it is built to cost almost nothing:
- the ground (thousands of tiny rectangles) is drawn once into a bitmap and re-blitted, rebuilt only once a minute
- text layouts are cached, and bookkeeping that doesn't change every frame (tasks due) is refreshed once a second
- the frame rate follows need: about 30 fps for a couple of seconds after you touch it, 8 fps while friends walk,
  2 fps when everything is still, 10 fps for a static panel, and 1 check per second while hidden
- the slow timer (reminders, saving) fires every 5 seconds, and idle memory is handed back to Windows after the panel
  has been closed for 15 seconds
- windows only grow to panel size while a panel is open

`tools/measure.sh` measures CPU and per-frame cost under Wine (relative numbers only):
`EXE=build-win/GoalFriends.exe SECS=20 bash tools/measure.sh`. Setting `GF_TRACE=<file>` makes the app log its average
draw and present time per frame every 10 seconds.

## Roster and stats

Every friend has the six Pokémon stats: HP, ATK, DEF, SPA (special attack), SPD (special defence) and SPE (speed).
Beat dungeons (Battle tab) for loot: held items that boost a friend. Every friend also has a passive ability (see the guide's Abilities page). Moves are Physical (ATK vs DEF), Special (SPA vs SPD) or Utility. `ROSTER.md` lists every friend with stats, role,
concept, areas, evolution chain, full move details and which moves are unique to them. It is generated from the game
data, so it always matches the game:

```bash
cmake --build build && ./build/gf_roster_doc > ROSTER.md
```

The tests enforce variety: every friend has its own stat spread and at least one move with mechanics nobody else has, no
two friends share a moveset, and friends from different lines never share more than two moves.

## Roster generator

Friends 49 to 150 come from `tools/gen_roster.py` (Python 3, no dependencies). Edit the tables in that file and run
`python3 tools/gen_roster.py` from the repo root to regenerate `src/core/roster_ext.inc` and `src/core/sprite_lines.inc`.
The generator refuses to run if two moves share a name.

## Diagnostics

- `GF_TRACE=<file>`: log renderer errors to a file.
- `GF_FONT=<family>`: override the UI font (default Segoe UI).
- `GF_HOUR=<hours>`: pretend it is this hour of the day (e.g. `GF_HOUR=18.5`) to preview the ground's day/night colours.

## Prebuilt exe

`prebuilt/GoalFriends.exe` is a statically linked MinGW build of the current source, so you can try the app without
installing Visual Studio. It is unsigned (SmartScreen may warn: More info > Run anyway) and is rebuilt by hand, so it
can lag behind the source.
