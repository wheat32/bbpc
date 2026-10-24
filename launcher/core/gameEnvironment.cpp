#include "gameEnvironment.h"

#include <QDir>
#include <QRegularExpression>

#include "onlineAddresses.h"

namespace
{
const QString ON = QStringLiteral("1");
const QString OFF = QStringLiteral("0");
const QString NET_GENERATION = QStringLiteral("20");
const QRegularExpression WHITESPACE(QStringLiteral("\\s+"));

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
    insertIfSet(env, QStringLiteral("BB_READBACKS"), s.readbacks);
    insertIfSet(env, QStringLiteral("BB_PREUPLOAD"), s.preupload);
    if (s.mangohud)
    {
        env.insert(QStringLiteral("MANGOHUD"), ON);
    }
    if (s.frameStats)
    {
        env.insert(QStringLiteral("BB_FRAME_STATS"), ON);
    }
    if (s.saveLog)
    {
        env.insert(QStringLiteral("BB_SAVE_LOG"), ON);
    }

    // The memory model: "auto" is run.sh's choice (the new one on every GPU); BB_PC_MODEL from
    // the shell or the extra variables still wins over it.
    if (s.memoryModel == QStringLiteral("new") || s.memoryModel == QStringLiteral("old"))
    {
        env.insert(QStringLiteral("BB_PC_MODEL"), flag(s.memoryModel == QStringLiteral("new")));
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

    // Synchronization and memory as released in 0.3 (run.sh: BB_AS_0_3), for comparisons.
    if (s.as03)
    {
        env.insert(QStringLiteral("BB_AS_0_3"), ON);
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
        env.insert(QStringLiteral("BB_NET_GEN"), NET_GENERATION);
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
