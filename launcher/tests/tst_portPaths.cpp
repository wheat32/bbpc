#include <QtTest/QtTest>

#include "portPaths.h"

namespace
{
void makePort(const QString& folder)
{
    QDir().mkpath(folder + QStringLiteral("/scripts"));
    QFile file(folder + QStringLiteral("/run.sh"));
    QVERIFY(file.open(QIODevice::WriteOnly));
}
} // namespace

class TstPortPaths : public QObject
{
    Q_OBJECT

private slots:
    void findPortDir_sourceBuild_isTheRepository()
    {
        QTemporaryDir root;
        makePort(root.path());
        QDir().mkpath(root.filePath(QStringLiteral("out/launcher")));
        QCOMPARE(PortPaths::findPortDir(root.filePath(QStringLiteral("out/launcher"))), QDir::cleanPath(root.path()));
    }

    void findPortDir_installed_isShareBbpc()
    {
        QTemporaryDir root;
        makePort(root.filePath(QStringLiteral("usr/share/bbpc")));
        QDir().mkpath(root.filePath(QStringLiteral("usr/bin")));
        QCOMPARE(PortPaths::findPortDir(root.filePath(QStringLiteral("usr/bin"))),
                 QDir::cleanPath(root.filePath(QStringLiteral("usr/share/bbpc"))));
    }

    void hasOnlineModule_netModule_isTakenAsItIs()
    {
        QTemporaryDir root;
        QVERIFY(PortPaths::hasOnlineModule({}, root.path()) == false);
        QDir().mkpath(root.filePath(QStringLiteral("gpu")));
        QFile module(root.filePath(QStringLiteral("gpu/libbbnet20.so")));
        QVERIFY(module.open(QIODevice::WriteOnly));
        module.close();
        QVERIFY(PortPaths::hasOnlineModule({}, root.path()));
        // BB_NET_MODULE names another file (the folder package's online/): that one counts.
        QVERIFY(PortPaths::hasOnlineModule(root.filePath(QStringLiteral("online/libbbnet20.so")), root.path()) == false);
    }

    void upstreamVersion_versionJson_isItsUpstreamKey()
    {
        QVERIFY(PortPaths::upstreamVersion(QByteArray()).isEmpty());
        QVERIFY(PortPaths::upstreamVersion(R"({"app_version": "0.7.3"})").isEmpty());
        QCOMPARE(PortPaths::upstreamVersion(R"({"app_version": "0.7.3", "upstream_version": "0.7"})"),
                 QStringLiteral("0.7"));
    }

    void findPortDir_nothingAround_isEmpty()
    {
        QTemporaryDir root;
        QDir().mkpath(root.filePath(QStringLiteral("a/b")));
        QVERIFY(PortPaths::findPortDir(root.filePath(QStringLiteral("a/b"))).isEmpty());
    }
};

QTEST_MAIN(TstPortPaths)
#include "tst_portPaths.moc"
