#include <QtTest/QtTest>

#include "onlineAddresses.h"

// The same answers as scripts/online.py gives (community_address, webapi_setting, thd_base).
class TstOnlineAddresses : public QObject
{
    Q_OBJECT

private slots:
    void communityAddress_empty_isSeamlessHunt()
    {
        QCOMPARE(OnlineAddresses::communityAddress(QString()), QStringLiteral("http://play.seamlesshunt.com"));
        QCOMPARE(OnlineAddresses::communityAddress(QStringLiteral("  ")), QStringLiteral("http://play.seamlesshunt.com"));
    }

    void communityAddress_defaultHostWithoutScheme_keepsPlainHttp()
    {
        QCOMPARE(OnlineAddresses::communityAddress(QStringLiteral("play.seamlesshunt.com")),
                 QStringLiteral("http://play.seamlesshunt.com"));
    }

    void communityAddress_otherHostWithoutScheme_isHttps()
    {
        QCOMPARE(OnlineAddresses::communityAddress(QStringLiteral("example.org:20443/bb-eu")),
                 QStringLiteral("https://example.org"));
    }

    void communityAddress_explicitScheme_isKept()
    {
        QCOMPARE(OnlineAddresses::communityAddress(QStringLiteral("http://10.0.0.2:8080/x")), QStringLiteral("http://10.0.0.2"));
    }

    void webapiAddress_hostAndPort_usesPort31315()
    {
        QCOMPARE(OnlineAddresses::webapiAddress(QStringLiteral("srv.example:31313")), QStringLiteral("http://srv.example:31315"));
        QCOMPARE(OnlineAddresses::webapiAddress(QString()), QStringLiteral("http://play.seamlesshunt.com:31315"));
    }

    void webapiSetting_gameServerEntered_fallsBackToShadnetWebapi()
    {
        QCOMPARE(OnlineAddresses::webapiSetting(QStringLiteral("https://example.org:18671"), QStringLiteral("srv.example:31313"),
                                                QStringLiteral("https://example.org")),
                 QStringLiteral("http://srv.example:31315"));
    }

    void webapiSetting_ownWebapi_isKept()
    {
        QCOMPARE(OnlineAddresses::webapiSetting(QStringLiteral(" http://api.example:9000 "), QStringLiteral("srv.example:31313"),
                                                QString()),
                 QStringLiteral("http://api.example:9000"));
    }

    void thdBase_variants_areSchemeAndHost()
    {
        QCOMPARE(OnlineAddresses::thdBase(QString()), QStringLiteral("https://thehuntersdream.com"));
        QCOMPARE(OnlineAddresses::thdBase(QStringLiteral("dream.example/")), QStringLiteral("https://dream.example"));
        QCOMPARE(OnlineAddresses::thdBase(QStringLiteral("http://dream.example/link")), QStringLiteral("http://dream.example"));
    }

    void migrateOldDefaults_oldAddresses_becomeCurrentDefaults()
    {
        QJsonObject settings {
            { QStringLiteral("online_server"), QStringLiteral("srv.shadps4.net:31313") },
            { QStringLiteral("online_community"), QStringLiteral("https://thehuntersdream.com") },
            { QStringLiteral("online_webapi"), QString() },
        };
        OnlineAddresses::migrateOldDefaults(settings);
        QCOMPARE(settings.value(QStringLiteral("online_server")).toString(), OnlineAddresses::SHADNET_SERVER);
        QCOMPARE(settings.value(QStringLiteral("online_community")).toString(), OnlineAddresses::COMMUNITY_SERVER);
        QCOMPARE(settings.value(QStringLiteral("online_webapi")).toString(), OnlineAddresses::WEBAPI_SERVER);
    }

    void migrateOldDefaults_ownServer_isKept()
    {
        QJsonObject settings { { QStringLiteral("online_server"), QStringLiteral("my.server:31313") } };
        OnlineAddresses::migrateOldDefaults(settings);
        QCOMPARE(settings.value(QStringLiteral("online_server")).toString(), QStringLiteral("my.server:31313"));
    }
};

QTEST_MAIN(TstOnlineAddresses)
#include "tst_onlineAddresses.moc"
