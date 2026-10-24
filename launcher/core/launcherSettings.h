#pragma once

#include <QJsonObject>
#include <QString>

#include "onlineAddresses.h"

// LauncherSettings - what the launcher remembers between runs (the port's own settings live in
// bbport.ini, PortIni). The JSON keys are the ones the GTK launcher of the original project used,
// so its settings.json is imported as it is. Keys this version does not know are kept.
struct LauncherSettings
{
    // fpsLimit when the frame rate is not capped.
    static constexpr int NO_FPS_LIMIT = 0;

    // Launcher
    QString theme = QStringLiteral("system");

    // Game
    QString gameDir;
    QString userDir;
    QString language = QStringLiteral("1");
    QString memoryModel = QStringLiteral("auto");
    QString gpu;

    // Mods and third-party patches
    QString modsDir;
    bool modsEnabled = true;
    QString patchesDir;

    // Display
    bool fullscreen = false;
    bool hdr = false;
    QString presentMode = QStringLiteral("Mailbox");
    QString display;

    // Controls
    QString gamepad;
    QString gamepadName;

    // Game patches and frame rate
    bool skipNetworkChoice = true;
    QString fpsMode = QStringLiteral("uncap");
    int fpsLimit = NO_FPS_LIMIT;

    // Performance
    QString drawPipe;
    QString readbacks;
    QString preupload;

    // Developer
    bool mangohud = false;
    bool frameStats = false;
    bool saveLog = false;
    bool crashDiag = false;
    bool as03 = false;
    bool gpuProfile = false;
    bool vkValidation = false;
    QString extraEnv;

    // Online
    bool online = false;
    bool onlineBbsc = true;
    QString onlineCommunity = OnlineAddresses::COMMUNITY_SERVER;
    QString onlineServer = OnlineAddresses::SHADNET_SERVER;
    QString onlineWebapi = OnlineAddresses::WEBAPI_SERVER;
    QString onlineNpid;
    QString onlinePassword;
    bool onlineUpnp = true;
    QString onlinePool = QStringLiteral("seamless");
    QString thdServer;
    QString thdOnlineId;
    QString thdToken;

    // FSR 4.1.1: the folder the last AMD DLL was chosen from.
    QString fsr411DllDir;

    // The saved object, unknown keys included; toJson() writes the known ones over it.
    QJsonObject saved;

    // The settings in json over the defaults (defaults.gameDir: the default game folder).
    static LauncherSettings fromJson(const QJsonObject& json);
    static LauncherSettings fromJson(const QJsonObject& json, const LauncherSettings& defaults);
    [[nodiscard]] QJsonObject toJson() const;

    // "huntersdream" or "seamless" (older settings named other pools: they are Seamless Hunt's).
    [[nodiscard]] QString pool() const;
};
