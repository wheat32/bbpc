#include <QtTest/QtTest>

#include "gameEnvironment.h"

namespace
{
const QList<Capabilities::Gpu> GPUS = {
    { 0, QStringLiteral("0x8086"), QStringLiteral("integrated"), QStringLiteral("Intel(R) UHD Graphics 630"), false },
    { 1, QStringLiteral("0x10de"), QStringLiteral("discrete"), QStringLiteral("NVIDIA GeForce RTX 4060 Laptop GPU"), true },
};

GameEnvironment::Location sourceTree()
{
    GameEnvironment::Location location;
    location.dataDir = QStringLiteral("/data");
    location.buildDir = QStringLiteral("/repo/out");
    return location;
}
} // namespace

class TstGameEnvironment : public QObject
{
    Q_OBJECT

private slots:
    void build_chosenGpu_isPassedByIndex()
    {
        LauncherSettings settings;
        settings.gpu = QStringLiteral("Intel(R) UHD Graphics 630");
        const QProcessEnvironment env = GameEnvironment::build(settings, {}, sourceTree(), GPUS);
        QCOMPARE(env.value(QStringLiteral("BB_GPU_ID")), QStringLiteral("0"));
    }

    void build_autoOrMissingGpu_leavesTheChoiceToThePort()
    {
        LauncherSettings settings;
        QVERIFY(GameEnvironment::build(settings, {}, sourceTree(), GPUS).contains(QStringLiteral("BB_GPU_ID")) == false);
        settings.gpu = QStringLiteral("AMD Radeon RX 580");
        QVERIFY(GameEnvironment::build(settings, {}, sourceTree(), GPUS).contains(QStringLiteral("BB_GPU_ID")) == false);
    }

    void build_shellGpuId_wins()
    {
        LauncherSettings settings;
        settings.gpu = QStringLiteral("Intel(R) UHD Graphics 630");
        QProcessEnvironment base;
        base.insert(QStringLiteral("BB_GPU_ID"), QStringLiteral("1"));
        QCOMPARE(GameEnvironment::build(settings, base, sourceTree(), GPUS).value(QStringLiteral("BB_GPU_ID")), QStringLiteral("1"));
    }

    void build_defaults_pointAtTheDataFolder()
    {
        LauncherSettings settings;
        settings.gameDir = QStringLiteral("/games/CUSA03173");
        const QProcessEnvironment env = GameEnvironment::build(settings, {}, sourceTree(), {});
        QCOMPARE(env.value(QStringLiteral("BB_GAME_DIR")), QStringLiteral("/games/CUSA03173"));
        QCOMPARE(env.value(QStringLiteral("BB_MODS_DIR")), QStringLiteral("/data/mods"));
        QCOMPARE(env.value(QStringLiteral("BB_PATCHES_CONFIG")), QStringLiteral("/data/patches.json"));
        QCOMPARE(env.value(QStringLiteral("BB_FPS")), QStringLiteral("uncap"));
        QCOMPARE(env.value(QStringLiteral("BB_BUILD_DIR")), QStringLiteral("/repo/out"));
        QVERIFY(env.contains(QStringLiteral("BB_PREBUILT")) == false);
        QVERIFY(env.contains(QStringLiteral("BB_ONLINE")) == false);
        QVERIFY(env.contains(QStringLiteral("BB_PC_MODEL")) == false);
    }

    void build_packaged_setsPrebuiltAndTheDataFolder()
    {
        GameEnvironment::Location location;
        location.packaged = true;
        location.dataDir = QStringLiteral("/home/me/.local/share/bbpc");
        const QProcessEnvironment env = GameEnvironment::build(LauncherSettings(), {}, location, {});
        QCOMPARE(env.value(QStringLiteral("BB_PREBUILT")), QStringLiteral("1"));
        QCOMPARE(env.value(QStringLiteral("BB_DATA_DIR")), location.dataDir);
        QVERIFY(env.contains(QStringLiteral("BB_BUILD_DIR")) == false);
    }

    void build_onlineSeamlessHunt_setsTheServersAndBbsc()
    {
        LauncherSettings settings;
        settings.online = true;
        settings.onlineNpid = QStringLiteral(" hunter ");
        GameEnvironment::Location location = sourceTree();
        location.onlineModule = true;
        const QProcessEnvironment env = GameEnvironment::build(settings, {}, location, {});
        QCOMPARE(env.value(QStringLiteral("BB_ONLINE")), QStringLiteral("1"));
        QCOMPARE(env.value(QStringLiteral("BB_SHADNET_SERVER")), QStringLiteral("play.seamlesshunt.com:31313"));
        QCOMPARE(env.value(QStringLiteral("BB_SHADNET_WEBAPI")), QStringLiteral("http://play.seamlesshunt.com:31315"));
        QCOMPARE(env.value(QStringLiteral("BB_SHADNET_NPID")), QStringLiteral("hunter"));
        QCOMPARE(env.value(QStringLiteral("BB_NET_GEN")), QStringLiteral("20"));
        QCOMPARE(env.value(QStringLiteral("BB_BBSC")), QStringLiteral("1"));
    }

    void build_onlineHuntersDream_usesItsBackend()
    {
        LauncherSettings settings;
        settings.online = true;
        settings.onlinePool = QStringLiteral("huntersdream");
        settings.thdOnlineId = QStringLiteral("id");
        settings.thdToken = QStringLiteral("token");
        GameEnvironment::Location location = sourceTree();
        location.onlineModule = true;
        const QProcessEnvironment env = GameEnvironment::build(settings, {}, location, {});
        QCOMPARE(env.value(QStringLiteral("BB_NET_BACKEND")), QStringLiteral("thd"));
        QCOMPARE(env.value(QStringLiteral("BB_COMMUNITY_SERVER")), QStringLiteral("https://thehuntersdream.com"));
        QCOMPARE(env.value(QStringLiteral("BB_THD_TOKEN")), QStringLiteral("token"));
        QVERIFY(env.contains(QStringLiteral("BB_BBSC")) == false);
    }

    void build_onlineWithoutTheModule_staysOffline()
    {
        LauncherSettings settings;
        settings.online = true;
        QVERIFY(GameEnvironment::build(settings, {}, sourceTree(), {}).contains(QStringLiteral("BB_ONLINE")) == false);
    }

    void build_extraVariablesAndModel_override()
    {
        LauncherSettings settings;
        settings.memoryModel = QStringLiteral("old");
        settings.extraEnv = QStringLiteral("BB_FRAME_STATS=1  BB_PC_MODEL=1 junk =x");
        const QProcessEnvironment env = GameEnvironment::build(settings, {}, sourceTree(), {});
        QCOMPARE(env.value(QStringLiteral("BB_FRAME_STATS")), QStringLiteral("1"));
        QCOMPARE(env.value(QStringLiteral("BB_PC_MODEL")), QStringLiteral("1"));
        QVERIFY(env.contains(QStringLiteral("junk")) == false);
    }

    void expandHome_tilde_isTheHomeFolder()
    {
        QCOMPARE(GameEnvironment::expandHome(QStringLiteral("~/games")), QDir::homePath() + QStringLiteral("/games"));
        QCOMPARE(GameEnvironment::expandHome(QStringLiteral("/abs")), QStringLiteral("/abs"));
    }
};

QTEST_MAIN(TstGameEnvironment)
#include "tst_gameEnvironment.moc"
