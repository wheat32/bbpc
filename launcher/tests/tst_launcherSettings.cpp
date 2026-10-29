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

    void checkForUpdates_onByDefault_andKeptWhenTurnedOff()
    {
        QVERIFY(LauncherSettings::fromJson({}).checkForUpdates);
        LauncherSettings settings = LauncherSettings::fromJson({ { QStringLiteral("check_for_updates"), false } });
        QVERIFY(settings.checkForUpdates == false);
        settings.checkForUpdates = true;
        QCOMPARE(settings.toJson().value(QStringLiteral("check_for_updates")).toBool(), true);
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

    void fromJson_removedSettings_areKeptButNotRead()
    {
        // 0.7 has one memory model and no 0.3 synchronization: older keys stay in the file.
        const QJsonObject json {
            { QStringLiteral("memory_model"), QStringLiteral("old") },
            { QStringLiteral("as_0_3"), true },
            { QStringLiteral("trophy_key"), QStringLiteral("00112233445566778899aabbccddeeff") },
        };
        const LauncherSettings settings = LauncherSettings::fromJson(json);
        QCOMPARE(settings.trophyKey, QStringLiteral("00112233445566778899aabbccddeeff"));
        QVERIFY(settings.mouseAutoRotate == false);
        const QJsonObject saved = settings.toJson();
        QCOMPARE(saved.value(QStringLiteral("memory_model")).toString(), QStringLiteral("old"));
        QCOMPARE(saved.value(QStringLiteral("trophy_key")).toString(), settings.trophyKey);
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
