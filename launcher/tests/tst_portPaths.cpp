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

    void findPortDir_nothingAround_isEmpty()
    {
        QTemporaryDir root;
        QDir().mkpath(root.filePath(QStringLiteral("a/b")));
        QVERIFY(PortPaths::findPortDir(root.filePath(QStringLiteral("a/b"))).isEmpty());
    }
};

QTEST_MAIN(TstPortPaths)
#include "tst_portPaths.moc"
