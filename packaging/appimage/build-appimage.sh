#!/usr/bin/env bash
# packaging/appimage/build-appimage.sh
# Builds the AppImage: the launcher, the port's prebuilt binaries and scripts, and the libraries
# they need. glibc, the Vulkan loader and the GPU drivers are the host's (as for any native game),
# and so are bash and Python 3 (run.sh prepares the game image with them).
#
# Usage:
#   bash packaging/appimage/build-appimage.sh          build only
#   bash packaging/appimage/build-appimage.sh --run    build, then start the AppImage
#
# Requirements: what packaging/arch/install-deps.sh installs, wget, and the network (linuxdeploy,
# the FSR 4 v07 assets, dxil-spirv's source). Options (environment):
#   BBPC_APPIMAGE_FSR4=0      leave out the FSR 4 v07 assets (tools/fetch_fsr4_assets.sh, MIT)
#   BBPC_APPIMAGE_FSR4CAP=0   leave out the prebuilt FSR 4.1.1 tools (dxil-spirv, fsr4cap.exe)
#   DLSS_SDK_ROOT=<dir>       with the DLSS bridge and NVIDIA's runtime library
#                             (bash tools/fetch_dlss_sdk.sh <dir> fetches the SDK)
#
# The AppImage runs on distributions whose glibc is at least as new as the one it was built on.
#
# Output:
#   dist/Bloodborne-bbpc-<version>-x86_64.AppImage

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
WORK="${ROOT}/.appimage-build"
APPDIR="${WORK}/AppDir"
TOOLS_DIR="${WORK}/tools"
BUILD_DIR="${WORK}/build"
OUTPUT_DIR="${ROOT}/dist"
APP_ID="io.github.wheat32.bbpc"
PORT_DIR="${APPDIR}/usr/share/bbpc"
DXIL_SPIRV_COMMIT="7dc52786cdb1f53c54dd5c5c2698dff00ea5f0a3"

VERSION=$(python3 -c "import json, sys; print(json.load(open(sys.argv[1]))['app_version'])" "${ROOT}/version.json")
OUTPUT="${OUTPUT_DIR}/Bloodborne-bbpc-${VERSION}-x86_64.AppImage"

# -- Helpers -------------------------------------------------------------------
GREEN='\033[0;32m'; RED='\033[0;31m'; NC='\033[0m'
info() { echo -e "${GREEN}[build-appimage]${NC} $*"; }
die()  { echo -e "${RED}[build-appimage] ERROR:${NC} $*" >&2; exit 1; }

for tool in cmake ninja python3 wget gcc strip git; do
    command -v "${tool}" >/dev/null 2>&1 || die "${tool} is not installed (bash packaging/arch/install-deps.sh)"
done
if [[ ${BBPC_APPIMAGE_FSR4CAP:-1} == 1 ]]; then
    command -v x86_64-w64-mingw32-gcc >/dev/null 2>&1 || die "x86_64-w64-mingw32-gcc is not installed (Arch: mingw-w64-gcc)"
fi

# -- FSR 4 v07 assets (MIT, built from AMD's FidelityFX source by Q2RTX) -----------------------
if [[ ${BBPC_APPIMAGE_FSR4:-1} == 1 ]]; then
    info "Fetching the FSR 4 v07 assets..."
    bash "${ROOT}/tools/fetch_fsr4_assets.sh"
fi

# -- Build and install ---------------------------------------------------------
# The port's binaries live in usr/share/bbpc/bin and bin/gpu; their RUNPATH names usr/lib from
# both depths, so they find the bundled libraries without LD_LIBRARY_PATH.
info "Building bbpc ${VERSION} (release preset)..."
cmake --preset release -B "${BUILD_DIR}" -DCMAKE_INSTALL_PREFIX=/usr \
      -DBBPC_PACKAGE_RPATH='$ORIGIN/../../../lib;$ORIGIN/../../../../lib' \
      -DDLSS_SDK_ROOT="${DLSS_SDK_ROOT:-}"
cmake --build "${BUILD_DIR}"

info "Installing into the AppDir..."
rm -rf "${APPDIR}"
DESTDIR="${APPDIR}" cmake --install "${BUILD_DIR}"
# With the DLSS SDK the package must carry DLSS: a release without it falls back to FSR silently.
if [[ -n ${DLSS_SDK_ROOT:-} ]]; then
    [[ -f "${PORT_DIR}/bin/libbbport_dlss.so" && -f "${PORT_DIR}/NVIDIA-DLSS-LICENSE.txt" ]] &&
        compgen -G "${PORT_DIR}/bin/libnvidia-ngx-dlss.so.*" >/dev/null ||
        die "DLSS_SDK_ROOT is set, but the DLSS bridge, NVIDIA's library or its license was not installed"
    info "DLSS: the bridge and NVIDIA's library are in"
    compgen -G "${PORT_DIR}/bin/libnvidia-ngx-dlssg.so.*" >/dev/null &&
        info "DLSS Frame Generation: NVIDIA's library is in"
fi
# Debug info names the build environment and is most of the binaries' size; the symbols stay
# for crash reports. NVIDIA's DLSS library stays as NVIDIA built it: NGX refuses a changed copy
# (FAIL_NotImplemented, DLSS reported as unsupported).
find "${PORT_DIR}/bin" -type f \( -name '*.so*' -o -perm -u+x \) ! -name 'libnvidia-ngx-dlss.so.*' \
     ! -name 'libnvidia-ngx-dlssg.so.*' -exec strip --strip-debug {} +
strip --strip-debug "${APPDIR}/usr/bin/bbpc"

# -- FSR 4.1.1 tools, prebuilt: the launcher's "Choose DLL..." then needs no compiler -----------
if [[ ${BBPC_APPIMAGE_FSR4CAP:-1} == 1 ]]; then
    info "Building the FSR 4.1.1 tools (dxil-spirv, fsr4cap.exe)..."
    fsr4cap="${ROOT}/tools/fsr4cap"
    dxil="${WORK}/dxil-spirv"
    if [[ ! -x "${dxil}/build/dxil-spirv" ]]; then
        rm -rf "${dxil}"
        git clone -q https://github.com/HansKristian-Work/dxil-spirv.git "${dxil}"
        git -C "${dxil}" checkout -q "${DXIL_SPIRV_COMMIT}"
        git -C "${dxil}" submodule update -q --init --recursive
        git -C "${dxil}" apply "${fsr4cap}/dxil-spirv-class-bindings.patch"
        cmake -S "${dxil}" -B "${dxil}/build" -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF >/dev/null
        cmake --build "${dxil}/build" --target dxil-spirv >/dev/null
    fi
    mkdir -p "${PORT_DIR}/tools/fsr4cap/bin"
    install -m755 "${dxil}/build/dxil-spirv" "${PORT_DIR}/tools/fsr4cap/bin/dxil-spirv"
    strip --strip-debug "${PORT_DIR}/tools/fsr4cap/bin/dxil-spirv"
    x86_64-w64-mingw32-gcc -std=c23 -O1 -Wall -I"${fsr4cap}/ffx/api/include" -I"${fsr4cap}/ffx/upscalers/include" \
        "${fsr4cap}/fsr4cap.c" "${fsr4cap}/capture.c" "${fsr4cap}/rootsig.c" \
        -o "${PORT_DIR}/tools/fsr4cap/bin/fsr4cap.exe" -ld3d12 -ldxguid -static
fi

# -- Desktop integration -------------------------------------------------------
DESKTOP_FILE="${APPDIR}/usr/share/applications/${APP_ID}.desktop"
ICON_FILE="${APPDIR}/usr/share/icons/hicolor/scalable/apps/${APP_ID}.svg"
[[ -f "${DESKTOP_FILE}" && -f "${ICON_FILE}" ]] || die "the launcher's desktop file or icon was not installed"

# -- linuxdeploy ---------------------------------------------------------------
LINUXDEPLOY="${TOOLS_DIR}/linuxdeploy-x86_64.AppImage"
LINUXDEPLOY_QT="${TOOLS_DIR}/linuxdeploy-plugin-qt-x86_64.AppImage"
APPIMAGETOOL="${TOOLS_DIR}/appimagetool-x86_64.AppImage"
mkdir -p "${TOOLS_DIR}"
declare -A TOOL_URLS=(
    ["${LINUXDEPLOY}"]="https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage"
    ["${LINUXDEPLOY_QT}"]="https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage"
    ["${APPIMAGETOOL}"]="https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-x86_64.AppImage"
)
for dest in "${!TOOL_URLS[@]}"; do
    if [[ ! -f "${dest}" ]]; then
        info "Downloading $(basename "${dest}")..."
        wget -q -O "${dest}" "${TOOL_URLS[${dest}]}"
        chmod +x "${dest}"
    fi
done
export APPIMAGE_EXTRACT_AND_RUN=1

QMAKE=$(command -v qmake6 2>/dev/null || command -v qmake 2>/dev/null || true)
[[ -n "${QMAKE}" ]] || die "qmake6 not found (Arch: qt6-base)"
export QMAKE

# linuxdeploy must not rewrite the port's own binaries: the game's loader is not position
# independent and patchelf corrupted its symbol versions before. Their RUNPATH is already right;
# they are put back as they were after it collected their libraries.
info "Bundling libraries..."
saved="${WORK}/port-bin"
rm -rf "${saved}"
cp -a "${PORT_DIR}/bin" "${saved}"
deploy_deps=(--deploy-deps-only "${PORT_DIR}/bin" --deploy-deps-only "${PORT_DIR}/bin/gpu")
if [[ -d "${PORT_DIR}/tools/fsr4cap/bin" ]]; then
    deploy_deps+=(--deploy-deps-only "${PORT_DIR}/tools/fsr4cap/bin/dxil-spirv")
fi
NO_STRIP=1 "${LINUXDEPLOY}" \
    --appdir "${APPDIR}" \
    --executable "${APPDIR}/usr/bin/bbpc" \
    --desktop-file "${DESKTOP_FILE}" \
    --icon-file "${ICON_FILE}" \
    "${deploy_deps[@]}"
rm -rf "${PORT_DIR}/bin"
cp -a "${saved}" "${PORT_DIR}/bin"

info "Bundling Qt..."
LINUXDEPLOY_QT_EXTRACTED="${WORK}/linuxdeploy-plugin-qt-extracted"
if [[ ! -d "${LINUXDEPLOY_QT_EXTRACTED}" ]]; then
    (cd "${WORK}" && "${LINUXDEPLOY_QT}" --appimage-extract >/dev/null)
    mv "${WORK}/squashfs-root" "${LINUXDEPLOY_QT_EXTRACTED}"
fi
# Qt's plugin folder without KDE's image format plugins (kimageformats installs them into Qt's
# own folder): the launcher shows SVG and PNG only, and some of them need libraries that are not
# installed (kimg_jxr: libjpegxr). The plugin step reads the folder from qmake, so it gets a qmake
# that names the copy.
QT_PLUGINS_DIR=$("${QMAKE}" -query QT_INSTALL_PLUGINS)
FILTERED_PLUGINS="${WORK}/qt-plugins"
rm -rf "${FILTERED_PLUGINS}"
cp -r "${QT_PLUGINS_DIR}" "${FILTERED_PLUGINS}"
rm -f "${FILTERED_PLUGINS}"/imageformats/kimg_*.so
QMAKE_WRAPPER="${WORK}/qmake"
cat > "${QMAKE_WRAPPER}" <<WRAPPER
#!/bin/sh
"${QMAKE}" "\$@" | sed -e "s|^QT_INSTALL_PLUGINS:.*|QT_INSTALL_PLUGINS:${FILTERED_PLUGINS}|" \\
                      -e "s|^${QT_PLUGINS_DIR}\$|${FILTERED_PLUGINS}|"
WRAPPER
chmod +x "${QMAKE_WRAPPER}"
QMAKE="${QMAKE_WRAPPER}" NO_STRIP=1 EXTRA_QT_MODULES="svg" \
    "${LINUXDEPLOY_QT_EXTRACTED}/usr/bin/linuxdeploy-plugin-qt" --appdir "${APPDIR}"

# The Wayland platform plugin, with the shell and decoration plugins it loads.
for platforms in /usr/lib/qt6/plugins/platforms /usr/lib/x86_64-linux-gnu/qt6/plugins/platforms /usr/lib64/qt6/plugins/platforms; do
    if [[ -f "${platforms}/libqwayland.so" ]]; then
        cp "${platforms}/libqwayland.so" "${APPDIR}/usr/plugins/platforms/"
        wayland_deps=(--deploy-deps-only "${APPDIR}/usr/plugins/platforms/libqwayland.so")
        plugins=$(dirname "${platforms}")
        for sub in wayland-shell-integration wayland-graphics-integration-client wayland-decoration-client; do
            if [[ -d "${plugins}/${sub}" ]]; then
                mkdir -p "${APPDIR}/usr/plugins/${sub}"
                cp "${plugins}/${sub}"/*.so "${APPDIR}/usr/plugins/${sub}/" 2>/dev/null || true
                wayland_deps+=(--deploy-deps-only "${APPDIR}/usr/plugins/${sub}")
            fi
        done
        NO_STRIP=1 "${LINUXDEPLOY}" --appdir "${APPDIR}" "${wayland_deps[@]}"
        info "Bundled the Wayland platform plugin (${platforms})"
        break
    fi
done

# Qt's TLS plugin (OpenSSL) for HTTPS: the launcher's update check reads version.json from GitHub.
for tls in /usr/lib/qt6/plugins/tls /usr/lib/x86_64-linux-gnu/qt6/plugins/tls /usr/lib64/qt6/plugins/tls; do
    if [[ -f "${tls}/libqopensslbackend.so" ]]; then
        mkdir -p "${APPDIR}/usr/plugins/tls"
        cp "${tls}/libqopensslbackend.so" "${APPDIR}/usr/plugins/tls/"
        NO_STRIP=1 "${LINUXDEPLOY}" --appdir "${APPDIR}" --deploy-deps-only "${APPDIR}/usr/plugins/tls"
        info "Bundled Qt's TLS plugin (${tls})"
        break
    fi
done
[[ -f "${APPDIR}/usr/plugins/tls/libqopensslbackend.so" ]] ||
    die "Qt's TLS plugin (tls/libqopensslbackend.so, Arch: qt6-base) was not found: the update check needs HTTPS"

# The host's Vulkan loader and drivers serve the game, as for any native game.
rm -f "${APPDIR}"/usr/lib/libvulkan.so*

# linuxdeploy made AppRun a link to usr/bin/bbpc: replaced, not written through.
rm -f "${APPDIR}/AppRun"
cp "${ROOT}/packaging/appimage/AppRun" "${APPDIR}/AppRun"
chmod +x "${APPDIR}/AppRun"

# NGX loads only NVIDIA's own file: the packaged copy must be the SDK's, byte for byte.
if [[ -n ${DLSS_SDK_ROOT:-} ]]; then
    for library in "${PORT_DIR}"/bin/libnvidia-ngx-dlss.so.* "${PORT_DIR}"/bin/libnvidia-ngx-dlssg.so.*; do
        [[ -e ${library} ]] || continue
        cmp -s "${library}" "${DLSS_SDK_ROOT}/lib/Linux_x86_64/rel/$(basename "${library}")" ||
            die "$(basename "${library}") in the AppDir differs from the DLSS SDK's: NGX would refuse it"
    done
fi

# -- AppImage ------------------------------------------------------------------
mkdir -p "${OUTPUT_DIR}"
info "Creating ${OUTPUT}..."
"${APPIMAGETOOL}" --no-appstream "${APPDIR}" "${OUTPUT}"
info "Done: ${OUTPUT} ($(du -h "${OUTPUT}" | cut -f1))"

if [[ "${1:-}" == "--run" ]]; then
    info "Starting ${OUTPUT}..."
    "${OUTPUT}"
fi
