#pragma once

#include <QProcessEnvironment>
#include <QString>
#include <QStringList>

// Fsr411Build - tools/fsr4cap/build_assets.sh for the user's own AMD DLL: the FSR 4.1.1 model is
// recorded under Proton and translated to SPIR-V (nothing of AMD's is downloaded or shipped). Its
// second form adds FSR 4.1.2's models (INT8 RDNA2, INT8 Lite) from AMD's own driver.
namespace Fsr411Build
{
    struct Command
    {
        QString program;
        QStringList arguments;
        QProcessEnvironment env;
        QString workingDirectory;
    };

    // loader: empty when the upscaler DLL exports the FidelityFX API itself. A package writes into
    // the data folder (it is read-only itself) and removes the work folder afterwards.
    Command command(const QString& upscalerDll, const QString& loaderDll);

    // FSR 4.1.2's models from AMD's driver: its amdxcffx64.dll or an AMD Adrenalin 26.10.1+
    // installer (.exe, unpacked with 7z), recorded through the FSR 4.1.x upscaler DLL.
    Command driverCommand(const QString& driver, const QString& upscalerDll);

    // What a failed build's exit status means (build_assets.sh documents them), translated.
    QString errorMessage(int exitCode);

    // The progress a line of the build's output shows, or an empty string.
    QString progress(const QString& line);
}
