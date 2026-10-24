#!/usr/bin/env bash
# Builds the port with CMake (CMakePresets.json): out/bb-probe, out/bb-gpu-capabilities,
# out/gpu/libbbgpu.so, the online module out/gpu/libbbnet20.so and the launcher out/launcher/bbpc.
#   bash build.sh                  the release preset (what run.sh plays)
#   bash build.sh --test           the same with the tests, then runs them
#   bash build.sh --preset debug   another preset (debug: out-debug/, launcher: out-launcher/)
# BB_BUILD_DIR=<a configured build folder>: rebuilds that folder instead (run.sh uses it to play
# another preset's build). BB_PGO: generate (an instrumented build that writes pgo/ while the game
# runs), use or off; by default the profile in pgo/ is used when there is one. BB_LTO=OFF disables
# link-time optimization. DLSS_SDK_ROOT=<github.com/NVIDIA/DLSS checkout> also builds the DLSS
# bridge (out/libbbport_dlss.so) and puts NVIDIA's libnvidia-ngx-dlss.so next to bb-probe.
set -euo pipefail
cd -- "$(dirname -- "$0")"
preset=release
run_tests=0
while (( $# )); do
    case $1 in
        --test) run_tests=1 ;;
        --preset) preset=${2:?--preset needs a name}; shift ;;
        *) echo "usage: build.sh [--test] [--preset release|debug|launcher]" >&2; exit 2 ;;
    esac
    shift
done

if [[ -n ${BB_BUILD_DIR:-} && ! -f ${BB_BUILD_DIR}/CMakeCache.txt ]]; then
    # A preset's folder that is not configured yet: that preset.
    case $(realpath -m -- "$BB_BUILD_DIR") in
        "$PWD/out") preset=release ;;
        "$PWD/out-debug") preset=debug ;;
        "$PWD/out-launcher") preset=launcher ;;
        *) echo "BB_BUILD_DIR=$BB_BUILD_DIR is not a configured build folder" >&2; exit 2 ;;
    esac
fi

missing=()
for tool in cmake ninja pkg-config git cc; do
    command -v "$tool" >/dev/null || missing+=("$tool")
done
if (( ${#missing[@]} )) || { [[ $preset != launcher ]] && ! pkg-config --exists vulkan sdl3; }; then
    echo "Missing build tools or libraries${missing[*]:+ (${missing[*]})}." >&2
    echo 'Arch Linux: bash packaging/arch/install-deps.sh installs everything needed.' >&2
    exit 1
fi

if [[ -n ${BB_BUILD_DIR:-} && -f ${BB_BUILD_DIR}/CMakeCache.txt ]]; then
    build=$BB_BUILD_DIR
else
    case $preset in
        release) build=out ;;
        debug) build=out-debug ;;
        launcher) build=out-launcher ;;
        *) echo "Unknown preset: $preset" >&2; exit 2 ;;
    esac
    # PGO: the profile in pgo/ when there is one (BB_PGO=generate writes it).
    pgo=${BB_PGO:-}
    if [[ -z $pgo ]]; then
        if [[ -n $(find pgo -name '*.gcda' -print -quit 2>/dev/null) ]]; then pgo=use; else pgo=off; fi
    fi
    mkdir -p pgo "$build"
    options=(-DBB_PGO="$pgo" -DDLSS_SDK_ROOT="${DLSS_SDK_ROOT:-}")
    [[ -z ${BB_LTO:-} ]] || options+=(-DBB_LTO="$BB_LTO")
    (( run_tests == 0 )) || options+=(-DBBPC_BUILD_TESTS=ON)
    if ! cmake --preset "$preset" "${options[@]}" > "$build/cmake.log" 2>&1; then
        tail -40 "$build/cmake.log" >&2
        echo "Configuring failed (full log: $build/cmake.log)" >&2; exit 1
    fi
    grep -e '^-- Online module' -e '^-- DLSS bridge' "$build/cmake.log" || true
    echo "GPU library: PGO $pgo, LTO ${BB_LTO:-preset default}"
fi

# A failed build must stop here: an older libbbgpu.so would otherwise be used silently.
if ! cmake --build "$build" > "$build/build.log" 2>&1; then
    grep -v '^\[' "$build/build.log" | tail -40 >&2
    echo "Build failed (full log: $build/build.log)" >&2; exit 1
fi
[[ ! -f $build/bb-probe ]] || echo "Built $PWD/$build/bb-probe"
[[ ! -f $build/launcher/bbpc ]] || echo "Built $PWD/$build/launcher/bbpc"

if (( run_tests )); then
    QT_QPA_PLATFORM=offscreen ctest --test-dir "$build" --output-on-failure --parallel "$(nproc)"
fi
