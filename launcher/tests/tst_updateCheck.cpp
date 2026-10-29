#include <QtTest/QtTest>

#include "updateCheck.h"

class TstUpdateCheck : public QObject
{
    Q_OBJECT

private slots:
    void remoteVersion_versionJson_isItsAppVersion()
    {
        QCOMPARE(UpdateCheck::remoteVersion(R"({"app_version": "1.0.1", "upstream_version": "0.7"})"),
                 QStringLiteral("1.0.1"));
    }

    void remoteVersion_noAppVersionOrNoJson_isEmpty()
    {
        QVERIFY(UpdateCheck::remoteVersion(R"({"upstream_version": "0.7"})").isEmpty());
        QVERIFY(UpdateCheck::remoteVersion("<html>404</html>").isEmpty());
        QVERIFY(UpdateCheck::remoteVersion({}).isEmpty());
    }

    void isNewer_laterRelease_isTrue()
    {
        QVERIFY(UpdateCheck::isNewer(QStringLiteral("1.0.1"), QStringLiteral("1.0.0")));
        QVERIFY(UpdateCheck::isNewer(QStringLiteral("1.1"), QStringLiteral("1.0.9")));
        QVERIFY(UpdateCheck::isNewer(QStringLiteral("1.0.0"), QStringLiteral("0.7.3")));
    }

    void isNewer_sameOrEarlierRelease_isFalse()
    {
        QVERIFY(UpdateCheck::isNewer(QStringLiteral("1.0.0"), QStringLiteral("1.0.0")) == false);
        QVERIFY(UpdateCheck::isNewer(QStringLiteral("0.7.3"), QStringLiteral("1.0.0")) == false);
    }

    void isNewer_trailingZeros_areTheSameVersion()
    {
        QVERIFY(UpdateCheck::isNewer(QStringLiteral("1.0.0"), QStringLiteral("1.0")) == false);
        QVERIFY(UpdateCheck::isNewer(QStringLiteral("1.0"), QStringLiteral("1.0.0")) == false);
    }

    void isNewer_notAVersion_isFalse()
    {
        QVERIFY(UpdateCheck::isNewer({}, QStringLiteral("1.0.0")) == false);
        QVERIFY(UpdateCheck::isNewer(QStringLiteral("unknown"), QStringLiteral("1.0.0")) == false);
        QVERIFY(UpdateCheck::isNewer(QStringLiteral("1.0.1"), {}) == false);
    }
};

QTEST_MAIN(TstUpdateCheck)
#include "tst_updateCheck.moc"
