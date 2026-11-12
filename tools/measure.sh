#!/usr/bin/env bash
# Rough resource measurement of the Windows build under Wine (relative numbers; Wine adds its own overhead).
# Usage: EXE=build-win/GoalFriends.exe SECS=20 tools/measure.sh   (needs wine64, xvfb, xdotool)
EXE=${EXE:-build-win/GoalFriends.exe}; SECS=${SECS:-20}
export WINEPREFIX=${WINEPREFIX:-/tmp/wp} DISPLAY=${DISPLAY:-:99} WINEDEBUG=-all GF_TRACE=/tmp/gf_perf.txt
HZ=$(getconf CLK_TCK)
cpu_ticks() { local t=0; for pid in $(pgrep -f "GoalFriends|wineserver"); do read -r -a f < /proc/$pid/stat 2>/dev/null && t=$((t + ${f[13]} + ${f[14]})); done; echo $t; }
rss_mb() { local k=0; for pid in $(pgrep -f "GoalFriends"); do k=$((k + $(awk '/VmRSS/{print $2}' /proc/$pid/status 2>/dev/null || echo 0))); done; echo $((k / 1024)); }
sample() { local t0 t1; t0=$(cpu_ticks); sleep "$SECS"; t1=$(cpu_ticks); echo "$1: CPU $(python3 -c "print(round(($t1-$t0)/$HZ/$SECS*100,1))")% of one core, app RSS $(rss_mb) MB"; }
pkill -f GoalFriends 2>/dev/null; rm -f "$GF_TRACE"
(timeout 300 /usr/lib/wine/wine64 "$EXE" >/dev/null 2>&1 &); sleep 14
W=$(xdotool search --name "Goal Friends" | tail -1); xdotool windowfocus "$W" 2>/dev/null
sample "strip only (idle)      "
xdotool mousemove 880 765 click 1; sleep 3
sample "Tasks panel open       "
xdotool mousemove 880 765 click 1; sleep 1
xdotool key ctrl+shift+semicolon; sleep 3
sample "hidden (hotkey)        "
xdotool key ctrl+shift+semicolon; sleep 1
grep -h perf "$GF_TRACE" 2>/dev/null | tail -4
pkill -f GoalFriends 2>/dev/null
