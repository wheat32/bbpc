#include "gameEnvironment.h"

#include <QDir>
#include <QRegularExpression>

#include "onlineAddresses.h"
#include "trophies.h"

namespace
{
const QString ON = QStringLiteral("1");
const QString OFF = QStringLiteral("0");
const QRegularExpression WHITESPACE(QStringLiteral("\\s+"));
// MangoHud's default lines and the game's RAM and VRAM; a MANGOHUD_CONFIG of the user's wins (set,
// it takes the place of MangoHud's config file).
const QString MANGOHUD_LINES = QStringLiteral("fps,frametime,frame_timing,gpu_stats,cpu_stats,ram,vram");

QString flag(bool value)
{
    return value ? ON : OFF;
}

void insertIfSet(QProcessEnvironment& env, const QString& name, const QString& value)
{
    if (value.isEmpty() == false)
    {
        env.insert(name, value);
    }
}
} // namespace

QString GameEnvironment::expandHome(const QString& path)
{
    if (path == QStringLiteral("~"))
    {
        return QDir::homePath();
    }
    if (path.startsWith(QStringLiteral("~/")))
    {
        return QDir::homePath() + path.mid(1);
    }
    return path;
}

QString GameEnvironment::modsDir(const LauncherSettings& settings, const QString& dataDir)
{
    return expandHome(settings.modsDir.isEmpty() ? dataDir + QStringLiteral("/mods") : settings.modsDir);
}

QString GameEnvironment::patchesDir(const LauncherSettings& settings, const QString& dataDir)
{
    return expandHome(settings.patchesDir.isEmpty() ? dataDir + QStringLiteral("/patches") : settings.patchesDir);
}

QProcessEnvironment GameEnvironment::build(const LauncherSettings& s, const QProcessEnvironment& base,
                                           const Location& location, const QList<Capabilities::Gpu>& gpus)
{
    QProcessEnvironment env = base;
    if (location.packaged)
    {
        env.insert(QStringLiteral("BB_PREBUILT"), ON);
        env.insert(QStringLiteral("BB_DATA_DIR"), location.dataDir);
    }
    else if (location.buildDir.isEmpty() == false && env.contains(QStringLiteral("BB_BUILD_DIR")) == false)
    {
        env.insert(QStringLiteral("BB_BUILD_DIR"), location.buildDir);
    }

    env.insert(QStringLiteral("BB_GAME_DIR"), expandHome(s.gameDir));
    insertIfSet(env, QStringLiteral("BB_USER_DIR"), expandHome(s.userDir));
    env.insert(QStringLiteral("BB_MODS_DIR"), modsDir(s, location.dataDir));
    env.insert(QStringLiteral("BB_MODS_CONFIG"), location.dataDir + QStringLiteral("/mods.json"));
    env.insert(QStringLiteral("BB_MODS_ENABLED"), flag(s.modsEnabled));
    env.insert(QStringLiteral("BB_PATCHES_DIR"), patchesDir(s, location.dataDir));
    env.insert(QStringLiteral("BB_PATCHES_CONFIG"), location.dataDir + QStringLiteral("/patches.json"));
    env.insert(QStringLiteral("BB_LANGUAGE"), s.language);
    env.insert(QStringLiteral("BB_FULLSCREEN"), flag(s.fullscreen));
    env.insert(QStringLiteral("BB_PRESENT_MODE"), s.presentMode);
    insertIfSet(env, QStringLiteral("BB_GAMEPAD"), s.gamepad);
    insertIfSet(env, QStringLiteral("BB_DISPLAY"), s.display);
    env.insert(QStringLiteral("BB_SKIP_NETWORK_CHOICE"), flag(s.skipNetworkChoice));
    if (s.hdr)
    {
        env.insert(QStringLiteral("BB_HDR"), ON);
    }
    env.insert(QStringLiteral("BB_FPS"), s.fpsMode);
    if (s.fpsLimit > LauncherSettings::NO_FPS_LIMIT)
    {
        env.insert(QStringLiteral("BB_FPS_LIMIT"), QString::number(s.fpsLimit));
    }
    insertIfSet(env, QStringLiteral("BB_DRAW_PIPE"), s.drawPipe);
    if (s.mangohud)
    {
        env.insert(QStringLiteral("MANGOHUD"), ON);
        if (env.contains(QStringLiteral("MANGOHUD_CONFIG")) == false)
        {
            env.insert(QStringLiteral("MANGOHUD_CONFIG"), MANGOHUD_LINES);
        }
    }
    if (s.frameStats)
    {
        env.insert(QStringLiteral("BB_FRAME_STATS"), ON);
    }
    if (s.saveLog)
    {
        env.insert(QStringLiteral("BB_SAVE_LOG"), ON);
    }

    // The game's camera turns back behind the character by itself; with the mouse the port keeps
    // it from doing so unless asked (src/runtime_pad/bindings.c).
    if (s.mouseAutoRotate)
    {
        env.insert(QStringLiteral("BB_MOUSE_AUTO_ROTATE"), ON);
    }

    // The GPU chosen by name: its index in Vulkan's order now. One not found (taken out, renamed
    // by a driver update) leaves the choice to the port.
    if (s.gpu.isEmpty() == false && base.contains(QStringLiteral("BB_GPU_ID")) == false)
    {
        for (const Capabilities::Gpu& gpu : gpus)
        {
            if (gpu.name == s.gpu)
            {
                env.insert(QStringLiteral("BB_GPU_ID"), QString::number(gpu.index));
                break;
            }
        }
    }

    if (s.crashDiag)
    {
        env.insert(QStringLiteral("BB_FREE_CHECK"), ON);
        env.insert(QStringLiteral("BB_WRITE_LOG"), ON);
        env.insert(QStringLiteral("BB_PRODUCER_CHECK"), ON);
    }
    if (s.gpuProfile)
    {
        env.insert(QStringLiteral("BB_GPU_PROFILE"), ON);
    }
    if (s.vkValidation)
    {
        env.insert(QStringLiteral("BB_VK_VALIDATION"), ON);
    }

    if (s.online && location.onlineModule)
    {
        const QString server = s.onlineServer.trimmed().isEmpty() ? OnlineAddresses::SHADNET_SERVER : s.onlineServer.trimmed();
        const QString community = OnlineAddresses::communityAddress(s.onlineCommunity);
        env.insert(QStringLiteral("BB_ONLINE"), ON);
        env.insert(QStringLiteral("BB_SHADNET_SERVER"), server);
        env.insert(QStringLiteral("BB_SHADNET_WEBAPI"), OnlineAddresses::webapiSetting(s.onlineWebapi, server, community));
        env.insert(QStringLiteral("BB_SHADNET_NPID"), s.onlineNpid.trimmed());
        env.insert(QStringLiteral("BB_SHADNET_PASSWORD"), s.onlinePassword);
        env.insert(QStringLiteral("BB_UPNP"), flag(s.onlineUpnp));
        env.insert(QStringLiteral("BB_COMMUNITY_SERVER"), community);
        if (s.pool() == QStringLiteral("seamless") && s.onlineBbsc)
        {
            env.insert(QStringLiteral("BB_BBSC"), ON);
        }
        if (s.pool() == QStringLiteral("huntersdream"))
        {
            // run.sh turns these into the backend's addresses and the host overrides.
            env.insert(QStringLiteral("BB_NET_BACKEND"), QStringLiteral("thd"));
            env.insert(QStringLiteral("BB_COMMUNITY_SERVER"), OnlineAddresses::thdBase(s.thdServer));
            env.insert(QStringLiteral("BB_THD_ONLINE_ID"), s.thdOnlineId);
            env.insert(QStringLiteral("BB_THD_TOKEN"), s.thdToken);
        }
    }

    // The trophy names and icons are decrypted with the player's own key (gpu/shim/bbport_trophies).
    if (Trophies::validKey(s.trophyKey))
    {
        env.insert(QStringLiteral("BB_TROPHY_KEY"), s.trophyKey.trimmed());
    }

    // The developer's extra variables: NAME=value, space-separated.
    for (const QString& item : s.extraEnv.split(WHITESPACE, Qt::SkipEmptyParts))
    {
        const qsizetype equals = item.indexOf(u'=');
        if (equals > 0)
        {
            env.insert(item.left(equals), item.mid(equals + 1));
        }
    }
    return env;
}
