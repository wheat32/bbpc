#include <QtTest/QtTest>

#include "launcherSettings.h"

class TstLauncherSettings : public QObject
{
    Q_OBJECT

private slots:
    void fromJson_empty_givesTheDefaults()
    {
        LauncherSettings defaults;
        defaults.gameDir = QStringLiteral("/games/CUSA03173");
        const LauncherSettings settings = LauncherSettings::fromJson({}, defaults);
        QCOMPARE(settings.gameDir, QStringLiteral("/games/CUSA03173"));
        QCOMPARE(settings.presentMode, QStringLiteral("Mailbox"));
        QVERIFY(settings.skipNetworkChoice);
        QCOMPARE(settings.pool(), QStringLiteral("seamless"));
    }

    void fromJson_originalLauncherSettings_areRead()
    {
        const QJsonObject json {
            { QStringLiteral("game_dir"), QStringLiteral("/g") },
            { QStringLiteral("fps_limit"), 72 },
            { QStringLiteral("mangohud"), true },
            { QStringLiteral("online_pool"), QStringLiteral("huntersdream") },
            { QStringLiteral("ui_language"), QStringLiteral("ru") },
        };
        const LauncherSettings settings = LauncherSettings::fromJson(json);
        QCOMPARE(settings.gameDir, QStringLiteral("/g"));
        QCOMPARE(settings.fpsLimit, 72);
        QVERIFY(settings.mangohud);
        QCOMPARE(settings.pool(), QStringLiteral("huntersdream"));
    }

    void fromJson_wrongTypes_keepTheDefaults()
    {
        const QJsonObject json { { QStringLiteral("fullscreen"), QStringLiteral("yes") }, { QStringLiteral("fps_limit"), QStringLiteral("60") } };
        const LauncherSettings settings = LauncherSettings::fromJson(json);
        QVERIFY(settings.fullscreen == false);
        QCOMPARE(settings.fpsLimit, LauncherSettings::NO_FPS_LIMIT);
    }

    void toJson_unknownKeys_areKept()
    {
        LauncherSettings settings = LauncherSettings::fromJson({ { QStringLiteral("ui_language"), QStringLiteral("ru") } });
        settings.hdr = true;
        const QJsonObject json = settings.toJson();
        QCOMPARE(json.value(QStringLiteral("ui_language")).toString(), QStringLiteral("ru"));
        QCOMPARE(json.value(QStringLiteral("hdr")).toBool(), true);
        QCOMPARE(json.value(QStringLiteral("online_server")).toString(), OnlineAddresses::SHADNET_SERVER);
    }

    void pool_oldPoolNames_areSeamlessHunt()
    {
        LauncherSettings settings;
        settings.onlinePool = QStringLiteral("shadnet");
        QCOMPARE(settings.pool(), QStringLiteral("seamless"));
    }
};

QTEST_MAIN(TstLauncherSettings)
#include "tst_launcherSettings.moc"
