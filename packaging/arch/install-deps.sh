#!/usr/bin/env bash
# Installs the Arch Linux packages bbpc needs (packaging/arch/packages.txt) with pacman.
# Works on Arch-based distributions too (EndeavourOS, CachyOS, ...). Run as a normal user: sudo is
# used when needed; inside a root container (the CI) it is not.
#   bash packaging/arch/install-deps.sh            the packages missing so far
#   bash packaging/arch/install-deps.sh --print    only print the list
set -euo pipefail
here=$(cd -- "$(dirname -- "$0")" && pwd)
mapfile -t packages < <(grep -v -e '^\s*#' -e '^\s*$' "$here/packages.txt")
if [[ ${1:-} == --print ]]; then
    printf '%s\n' "${packages[@]}"
    exit 0
fi
command -v pacman >/dev/null || { echo 'pacman not found: this script is for Arch Linux.' >&2; exit 1; }
if (( EUID == 0 )); then
    pacman -S --needed --noconfirm "${packages[@]}"
else
    sudo pacman -S --needed "${packages[@]}"
fi
