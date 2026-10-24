#pragma once

#include <QFileInfoList>
#include <QString>

// UpscalerAssets - what the upscalers need beyond the port itself: the FSR 4.1.1 model built from
// the user's own AMD DLL (tools/fsr4cap), and NVIDIA's DLSS runtime library.
namespace UpscalerAssets
{
    // The folder the FSR 4.1.1 assets are read from, as run.sh looks for them.
    QString fsr411Dir(const QString& portDir, const QString& dataDir);

    // What is missing for the FSR 4.1.1 model an output and preset need ("WxH", preset index), or
    // an empty string when it is all there. Outputs above 1080p use the 2160p model; Ultra
    // Performance (4) its own variant.
    QString fsr411Problem(const QString& directory, const QString& output, int preset);

    // NVIDIA's DLSS runtime: libnvidia-ngx-dlss.so.<version>, from the DLSS SDK
    // (lib/Linux_x86_64/rel); a game's nvngx_dlss.dll is Windows-only.
    inline const QString NGX_PREFIX = QStringLiteral("libnvidia-ngx-dlss.so.");
    inline const QString DLSS_SDK_URL = QStringLiteral("https://github.com/NVIDIA/DLSS/tree/main/lib/Linux_x86_64/rel");

    // The DLSS runtime libraries in directory, the newest version last.
    QFileInfoList ngxLibraries(const QString& directory);

    // The version part of a libnvidia-ngx-dlss.so.<version> file name.
    QString ngxVersion(const QFileInfo& library);

    // True when the file starts like an ELF binary.
    bool isElf(const QString& path);
}
