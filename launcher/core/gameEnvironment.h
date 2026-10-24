#pragma once

#include <QList>
#include <QProcessEnvironment>
#include <QString>

#include "capabilities.h"
#include "launcherSettings.h"

// GameEnvironment - the environment run.sh starts the game with, from the launcher's settings.
// run.sh and the runtime read the BB_* variables (README: "Useful variables").
namespace GameEnvironment
{
    // Where the port is and how run.sh should find it.
    struct Location
    {
        QString dataDir;      // mods.json, patches.json, the default mods/ and patches/ folders
        bool packaged = false; // BB_PREBUILT=1 and BB_DATA_DIR for run.sh
        QString buildDir;     // from source: the build folder to play (BB_BUILD_DIR)
        bool onlineModule = false; // the build has the online module (gpu/libbbnet20.so)
    };

    // base: the environment to start from (the host's). gpus: bb-gpu-capabilities --gpus, to pass
    // the GPU chosen by name as its index now (BB_GPU_ID; one not found leaves the choice to the
    // port, and BB_GPU_ID from the shell wins).
    QProcessEnvironment build(const LauncherSettings& settings, const QProcessEnvironment& base,
                              const Location& location, const QList<Capabilities::Gpu>& gpus);

    // A path with a leading ~ expanded.
    QString expandHome(const QString& path);

    // The mods folder and the third-party patches folder (data folder defaults).
    QString modsDir(const LauncherSettings& settings, const QString& dataDir);
    QString patchesDir(const LauncherSettings& settings, const QString& dataDir);
}
