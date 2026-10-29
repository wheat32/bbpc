#!/usr/bin/env bash
# tools/fetch_dlss_sdk.sh [DIR]: the parts of NVIDIA's DLSS SDK (github.com/NVIDIA/DLSS) that the
# DLSS bridge needs, at a pinned release, into DIR (default: dlss-sdk in the checkout): the
# headers, the static NGX library the bridge links, NVIDIA's runtime libraries (Super Resolution and
# Frame Generation) and their license.
# Then build with DLSS_SDK_ROOT=DIR (build.sh, packaging/appimage/build-appimage.sh).
# The SDK is NVIDIA's (LICENSE.txt): its runtime library may ship inside an application, with
# that license next to it, never on its own.
set -euo pipefail
cd -- "$(dirname -- "$0")/.."
tag=v310.9.16
commit=9981c6350dc58c0ce21f024fd65e31e851c31689
dest=$(realpath -m -- "${1:-dlss-sdk}")
if [[ -e $dest ]]; then
    if [[ $(git -C "$dest" rev-parse HEAD 2>/dev/null) == "$commit" ]]; then
        echo "DLSS SDK: $tag already in $dest"
        exit 0
    fi
    echo "DLSS SDK: $dest exists but is not NVIDIA's DLSS $tag; remove it or choose another folder" >&2
    exit 1
fi
# Only the trees, then only the files the build reads (~60 MB instead of the whole SDK).
git -c advice.detachedHead=false clone --quiet --depth 1 --branch "$tag" --filter=blob:none --sparse \
    https://github.com/NVIDIA/DLSS "$dest"
if [[ $(git -C "$dest" rev-parse HEAD) != "$commit" ]]; then
    echo "DLSS SDK: the tag $tag is no longer commit $commit" >&2
    rm -rf -- "$dest" # the clone made just now
    exit 1
fi
git -C "$dest" sparse-checkout set --no-cone /include/ /lib/Linux_x86_64/libnvsdk_ngx.a \
    '/lib/Linux_x86_64/rel/libnvidia-ngx-dlss.so.*' '/lib/Linux_x86_64/rel/libnvidia-ngx-dlssg.so.*' /LICENSE.txt
echo "DLSS SDK: $tag in $dest"
