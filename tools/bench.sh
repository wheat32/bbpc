#!/usr/bin/env bash
# tools/bench.sh NAME [key=value...]: a repeatable in-game benchmark. Copies a save and its
# settings into out/bench/NAME_<time>/data (the player's own files are never written), applies the
# key=value lines to that copy of bbport.ini, starts the game, chooses Continue on the title,
# waits until the level draws, stands still for ${HOLD:-40} s, closes the game and prints:
# FPS (average, median, 1% low), the frame time spread, CPU use per thread and GPU utilization.
# Run it from a terminal of the desktop the game should open on; it closes the game itself.
#   BB_BENCH_FROM=<data folder>  where the save comes from (default: the first of
#                                ~/.local/share/bbpc and ~/.local/share/bbport with user/savedata)
#   BB_GAME_DIR, BB_* ...        passed on to run.sh (e.g. BB_GPU_PROFILE=1, BB_SECTIONS=1)
# Example: tools/bench.sh liveoff live_resolution=0 upscaler=dlss preset=1
set -u
cd -- "$(dirname -- "$0")/.."
repo=$PWD
name=${1:?usage: tools/bench.sh NAME [key=value...]}
shift
seconds=${HOLD:-40}
from=${BB_BENCH_FROM:-}
if [[ -z $from ]]; then
    for candidate in "$HOME/.local/share/bbpc" "$HOME/.local/share/bbport"; do
        if [[ -d $candidate/user/savedata ]]; then from=$candidate; break; fi
    done
fi
[[ -n $from && -d $from/user/savedata ]] || { echo "bench: no save found (set BB_BENCH_FROM)" >&2; exit 1; }
game=${BB_GAME_DIR:-}
if [[ -z $game ]]; then
    if [[ -f game/CUSA03173/eboot.bin ]]; then game=$repo/game/CUSA03173; else game=$(realpath -m ../CUSA03173); fi
fi
out=$repo/out/bench/${name}_$(date +%m%d_%H%M%S)
mkdir -p "$out/data"
cp -r "$from/user" "$out/data/"
for file in bbport.ini mods.json patches.json; do
    [[ -f $from/$file ]] && cp "$from/$file" "$out/data/"
done
touch "$out/data/bbport.ini"
# No mouse look: a mouse moving over the window must not turn the camera.
for kv in mouse_look=0 "$@"; do
    sed -i "/^${kv%%=*}=/d" "$out/data/bbport.ini"
    echo "$kv" >> "$out/data/bbport.ini"
done
pad=$out/pad
: > "$pad"
BB_GAME_DIR=$game BB_DATA_DIR=$out/data BB_PAD_FILE=$pad BB_FRAME_STATS=1 BB_FRAME_LOG=$out/frames.csv \
    setsid timeout $((seconds + 240)) bash run.sh > "$out/run.log" 2>&1 &
group=$!
stop() {
    kill -TERM -- -"$group" 2>/dev/null
    sleep 3
    kill -KILL -- -"$group" 2>/dev/null
    echo "bench: closed the game at $(date +%T)"
}
press() {
    echo cross > "$pad"
    sleep 0.4
    : > "$pad"
}
echo "bench: starting the game ($out)"
waited=0
until grep -q "a game menu is open" "$out/run.log" 2>/dev/null; do
    sleep 1
    waited=$((waited + 1))
    if (( waited >= 180 )) || ! kill -0 "$group" 2>/dev/null; then
        echo "bench: the title screen did not come (see $out/run.log)"
        stop
        exit 1
    fi
done
sleep 4
press
# The level: a run of frames with hundreds of draws (the title draws a few dozen).
waited=0
until python3 -I tools/bench_stats.py "$out/frames.csv" --ready 2>/dev/null; do
    sleep 1
    waited=$((waited + 1))
    if (( waited >= 150 )) || ! kill -0 "$group" 2>/dev/null; then
        echo "bench: the level did not load (see $out/run.log)"
        stop
        exit 1
    fi
    # Once more, in case the first press only woke the menu.
    if (( waited == 20 )); then press; fi
done
sleep 5 # past the first frames' pipeline compiles
start=$(tail -n 1 "$out/frames.csv" | cut -d, -f1)
probe=$(pgrep -n -x bb-probe)
threads() {
    for task in /proc/"$probe"/task/*; do
        # utime + stime: fields 14 and 15, counted after the "(comm)" field, which may hold spaces.
        echo "$(cat "$task/comm" 2>/dev/null)|$(sed -E 's/^.*\) //' "$task/stat" 2>/dev/null | awk '{print $12 + $13}')|${task##*/}"
    done
}
threads > "$out/threads.start"
smi=
if command -v nvidia-smi > /dev/null; then
    nvidia-smi --query-gpu=utilization.gpu --format=csv,noheader,nounits -lms 500 > "$out/gpu.csv" 2>/dev/null &
    smi=$!
fi
sleep "$seconds"
threads > "$out/threads.end"
[[ -z $smi ]] || kill "$smi" 2>/dev/null
stop
python3 -I tools/bench_stats.py "$out/frames.csv" --from "$start" --seconds "$seconds" \
    --threads "$out/threads.start" "$out/threads.end" --gpu "$out/gpu.csv"
