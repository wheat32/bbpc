#!/usr/bin/env bash
# Adds this checkout's launcher to the application menu (~/.local/share/applications), with its
# icon. Build it first: bash build.sh.
set -euo pipefail
here=$(cd -- "$(dirname -- "$0")" && pwd)
data=${XDG_DATA_HOME:-$HOME/.local/share}
mkdir -p "$data/applications" "$data/icons/hicolor/scalable/apps"
sed "s|^Exec=.*|Exec=$here/bb-launcher.sh|" "$here/io.github.wheat32.bbpc.desktop" \
    > "$data/applications/io.github.wheat32.bbpc.desktop"
cp "$here/assets/io.github.wheat32.bbpc.svg" "$data/icons/hicolor/scalable/apps/"
echo "Installed $data/applications/io.github.wheat32.bbpc.desktop"
