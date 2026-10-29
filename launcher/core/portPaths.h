#pragma once

#include <QString>

// PortPaths - where the port lives (run.sh, scripts/, tools/), where its binaries are and where
// it writes (bbport.ini, saves, generated files).
//
// From source the launcher is <build folder>/launcher/bbpc: the port is the repository above the
// build folder, and the binaries are the ones built next to the launcher (out/ for the release
// preset). Installed (the AppImage, a release tarball) the launcher is <prefix>/bin/bbpc, the port
// is <prefix>/share/bbpc with its prebuilt binaries in share/bbpc/bin, and the data lives in
// ~/.local/share/bbpc. BBPC_PORT_DIR names the port folder explicitly; BB_PREBUILT, BB_DATA_DIR
// and BB_CONFIG mean what they mean to run.sh.
namespace PortPaths
{
    // run.sh, scripts/, tools/ and, when packaged, bin/.
    QString portDir();

    // True when the port's binaries are prebuilt (portDir()/bin) and run.sh builds nothing.
    bool packaged();

    // bb-probe, bb-gpu-capabilities, gpu/libbbgpu.so, gpu/libbbnet20.so.
    QString binariesDir();

    // The build folder run.sh should play from source (BB_BUILD_DIR); empty when packaged.
    QString buildDir();

    // Writable: bbport.ini, user/ (saves), mods/, patches/, out/ (generated files).
    QString dataDir();

    QString iniPath();
    QString capabilitiesTool();
    QString fsr4capDir();
    QString python();

    // The online module (libbbnet20.so, both pools): the one BB_NET_MODULE names (run.sh takes it
    // as it is), else gpu/libbbnet20.so next to the port's binaries; true when it is there.
    bool hasOnlineModule();
    bool hasOnlineModule(const QString& netModule, const QString& binariesDir);

    // The bbport release bbpc is based on ("0.7": upstream_version in version.json, embedded as a
    // resource), empty when it is not known.
    QString portVersion();
    QString upstreamVersion(const QByteArray& versionJson);

    // The port folder for a launcher in applicationDir, or an empty string (exposed for tests).
    QString findPortDir(const QString& applicationDir);

    // True when folder holds run.sh and scripts/.
    bool isPortDir(const QString& folder);
}
