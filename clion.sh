#!/usr/bin/env bash
# Opens this checkout in CLion. CLion loads the CMake presets (CMakePresets.json: Release, Debug,
# Launcher only, CI) as its CMake profiles and the run configurations in .run/ (the launcher, the
# game through run.sh, the release and debug builds, the tests). The submodules are checked out
# first, so that the first CMake configuration finds them.
#   bash clion.sh            CLion from PATH, the AUR/JetBrains package, Toolbox or Flatpak
#   CLION=/path/to/clion bash clion.sh
set -euo pipefail
root=$(cd -- "$(dirname -- "$0")" && pwd)
if [[ -d $root/.git ]] && [[ ! -f $root/gpu/third_party/fsr-vulkan/CMakeLists.txt || ! -f $root/gpu/third_party/imgui/imgui.h ]]; then
    git -C "$root" submodule update --init --recursive
fi
clion=${CLION:-}
if [[ -z $clion ]]; then
    for candidate in clion clion.sh /opt/clion/bin/clion /opt/clion/bin/clion.sh \
                     "$HOME/.local/share/JetBrains/Toolbox/scripts/clion" "$HOME/.local/bin/clion"; do
        if command -v "$candidate" >/dev/null 2>&1; then clion=$(command -v "$candidate"); break; fi
    done
fi
if [[ -z $clion ]] && command -v flatpak >/dev/null && flatpak info com.jetbrains.CLion >/dev/null 2>&1; then
    clion="flatpak run com.jetbrains.CLion"
fi
if [[ -z $clion ]]; then
    echo 'CLion not found. Install it (Arch: the AUR package clion, or JetBrains Toolbox), or set CLION=/path/to/clion.' >&2
    exit 1
fi
echo "Opening $root in $clion"
# CLion keeps running after this script: its output goes to its own log, not this terminal.
# shellcheck disable=SC2086
nohup $clion "$root" >/dev/null 2>&1 &
