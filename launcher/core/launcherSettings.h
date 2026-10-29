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
    // A look for a newer bbpc release at start (UpdateCheck).
    bool checkForUpdates = true;

    // Game
    QString gameDir;
    QString userDir;
    // The PS4 system language the game sees (BB_LANGUAGE): its menus, its confirm button.
    QString language = QStringLiteral("1");
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
    // The game's own camera auto-rotation also while the mouse turns the camera
    // (BB_MOUSE_AUTO_ROTATE=1; off in the port since 0.7).
    bool mouseAutoRotate = false;

    // Game patches and frame rate
    bool skipNetworkChoice = true;
    QString fpsMode = QStringLiteral("uncap");
    int fpsLimit = NO_FPS_LIMIT;

    // Performance
    QString drawPipe;

    // Developer
    bool mangohud = false;
    bool frameStats = false;
    bool saveLog = false;
    bool crashDiag = false;
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

    // Trophies: the player's trophy key (32 hexadecimal digits, the one shadPS4 asks for), with
    // which the game decrypts the trophy names and icons (BB_TROPHY_KEY).
    QString trophyKey;

    // FSR 4.1.1: the folder the last AMD DLL was chosen from, and that DLL (FSR 4.1.2's driver
    // models are recorded through it).
    QString fsr411DllDir;
    QString fsr411Dll;

    // The saved object, unknown keys included; toJson() writes the known ones over it.
    QJsonObject saved;

    // The settings in json over the defaults (defaults.gameDir: the default game folder).
    static LauncherSettings fromJson(const QJsonObject& json);
    static LauncherSettings fromJson(const QJsonObject& json, const LauncherSettings& defaults);
    [[nodiscard]] QJsonObject toJson() const;

    // "huntersdream" or "seamless" (older settings named other pools: they are Seamless Hunt's).
    [[nodiscard]] QString pool() const;
};
