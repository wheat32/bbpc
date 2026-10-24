#!/usr/bin/env bash
# Starts the launcher from a source checkout, building the port first when it has not been built
# (bash build.sh). Arguments are passed on: --play starts the game without the window.
set -euo pipefail
root=$(cd -- "$(dirname -- "$0")/.." && pwd)
launcher=$root/out/launcher/bbpc
if [[ ! -x $launcher ]]; then
    bash "$root/build.sh"
fi
exec "$launcher" "$@"
