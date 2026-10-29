#include <QtTest/QtTest>

#include "desktopShortcut.h"

class TstDesktopShortcut : public QObject
{
    Q_OBJECT

private slots:
    void playCommand_appImage_isPreferred()
    {
        QCOMPARE(DesktopShortcut::playCommand(QStringLiteral("/apps/bbpc.AppImage"), QStringLiteral("/tmp/.mount/usr/bin/bbpc")),
                 QStringList({ QStringLiteral("/apps/bbpc.AppImage"), QStringLiteral("--play") }));
        QCOMPARE(DesktopShortcut::playCommand(QString(), QStringLiteral("/repo/out/launcher/bbpc")).first(),
                 QStringLiteral("/repo/out/launcher/bbpc"));
    }

    void execLine_plainPath_isLeftAsItIs()
    {
        QCOMPARE(DesktopShortcut::execLine({ QStringLiteral("/usr/bin/bbpc"), QStringLiteral("--play") }),
                 QStringLiteral("/usr/bin/bbpc --play"));
    }

    void execLine_reservedCharacters_areQuotedAndEscaped()
    {
        // A space quotes the argument; $ is escaped inside the quotes, and that backslash is
        // doubled as a string escape; a literal % is %%.
        QCOMPARE(DesktopShortcut::execLine({ QStringLiteral("/home/me/My Games/bb$1.AppImage"), QStringLiteral("--play") }),
                 QStringLiteral("\"/home/me/My Games/bb\\\\$1.AppImage\" --play"));
        QCOMPARE(DesktopShortcut::execLine({ QStringLiteral("/a/100%/bbpc") }), QStringLiteral("/a/100%%/bbpc"));
    }

    void entry_hasTheCommandAndTheIcon()
    {
        const QString text = DesktopShortcut::entry({ QStringLiteral("/usr/bin/bbpc"), QStringLiteral("--play") },
                                                    QStringLiteral("/icons/bbpc.svg"));
        QVERIFY(text.startsWith(QStringLiteral("[Desktop Entry]\n")));
        QVERIFY(text.contains(QStringLiteral("\nExec=/usr/bin/bbpc --play\n")));
        QVERIFY(text.contains(QStringLiteral("\nIcon=/icons/bbpc.svg\n")));
        QVERIFY(text.contains(QStringLiteral("\nCategories=Game;\n")));
    }

    void folders_homeAsDesktop_isOnlyTheMenu()
    {
        QTemporaryDir home;
        const QString applications = home.filePath(QStringLiteral(".local/share/applications"));
        QCOMPARE(DesktopShortcut::folders(applications, home.path(), home.path()), QStringList({ applications }));
        QCOMPARE(DesktopShortcut::folders(applications, home.filePath(QStringLiteral("Desktop")), home.path()).size(), 1);
        QDir().mkpath(home.filePath(QStringLiteral("Desktop")));
        QCOMPARE(DesktopShortcut::folders(applications, home.filePath(QStringLiteral("Desktop")), home.path()).last(),
                 home.filePath(QStringLiteral("Desktop")));
    }

    void write_entry_isAnExecutableFileInEachFolder()
    {
        QTemporaryDir home;
        const QString applications = home.filePath(QStringLiteral("applications"));
        const DesktopShortcut::Result result = DesktopShortcut::write(QStringLiteral("[Desktop Entry]\n"), { applications });
        QVERIFY(result.error.isEmpty());
        QCOMPARE(result.written, QStringList({ QDir(applications).filePath(DesktopShortcut::FILE_NAME) }));
        QVERIFY(QFileInfo(result.written.first()).isExecutable());
    }

    void writeIcon_readOnlyOldIcon_isReplaced()
    {
        QTemporaryDir dir;
        const QString path = dir.filePath(QStringLiteral("icons/") + DesktopShortcut::ICON_NAME);
        QVERIFY(DesktopShortcut::writeIcon(QByteArrayLiteral("<svg/>"), path));
        QFile::setPermissions(path, QFileDevice::ReadOwner);
        QVERIFY(DesktopShortcut::writeIcon(QByteArrayLiteral("<svg></svg>"), path));
        QFile file(path);
        QVERIFY(file.open(QIODevice::ReadOnly));
        QCOMPARE(file.readAll(), QByteArrayLiteral("<svg></svg>"));
    }
};

QTEST_MAIN(TstDesktopShortcut)
#include "tst_desktopShortcut.moc"
