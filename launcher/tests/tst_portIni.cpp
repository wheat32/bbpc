#include <QtTest/QtTest>

#include "portIni.h"

class TstPortIni : public QObject
{
    Q_OBJECT

private slots:
    void parse_missingFile_givesDefaults()
    {
        PortIni ini;
        ini.parse(QString());
        QCOMPARE(ini.value(QStringLiteral("upscaler")), QStringLiteral("fsr4"));
        QCOMPARE(ini.value(QStringLiteral("effect_ssr")), QStringLiteral("0"));
        QCOMPARE(ini.value(QStringLiteral("output_res")), QStringLiteral("auto"));
        QCOMPARE(ini.value(QStringLiteral("low_latency")), QStringLiteral("on"));
        QVERIFY(ini.inFile(QStringLiteral("upscaler")) == false);
    }

    void defaults_removedSettings_areNotWritten()
    {
        // Live resolution changes are always on and the game's AA follows anti_aliasing: neither
        // key is the launcher's any more.
        PortIni ini;
        ini.parse(QString());
        QVERIFY(ini.contains(QStringLiteral("live_resolution")) == false);
        QVERIFY(ini.contains(QStringLiteral("effect_game_aa")) == false);
        QVERIFY(ini.serialize().contains(QStringLiteral("live_resolution")) == false);
    }

    void saveChanges_oldLiveResolutionKey_staysAsItIs()
    {
        QTemporaryDir dir;
        const QString path = dir.filePath(QStringLiteral("bbport.ini"));
        PortIni shown;
        shown.parse(QStringLiteral("live_resolution=0\n"));
        QVERIFY(shown.save(path));
        PortIni edited = shown;
        edited.setValue(QStringLiteral("preset"), QStringLiteral("2"));
        QVERIFY(PortIni::saveChanges(path, shown, edited));
        PortIni result;
        result.load(path);
        QCOMPARE(result.value(QStringLiteral("live_resolution")), QStringLiteral("0"));
        QCOMPARE(result.value(QStringLiteral("preset")), QStringLiteral("2"));
    }

    void serialize_editedKey_isRewrittenInPlaceAndCommentsStay()
    {
        PortIni ini;
        ini.parse(QStringLiteral("# mine\nupscaler = fsr3\ncustom=1\n"));
        QCOMPARE(ini.value(QStringLiteral("upscaler")), QStringLiteral("fsr3"));
        ini.setValue(QStringLiteral("upscaler"), QStringLiteral("taa"));
        const QStringList lines = ini.serialize().split(u'\n');
        QCOMPARE(lines.at(0), QStringLiteral("# mine"));
        QCOMPARE(lines.at(1), QStringLiteral("upscaler=taa"));
        QCOMPARE(lines.at(2), QStringLiteral("custom=1"));
        QVERIFY(lines.contains(QStringLiteral("preset=4")));
    }

    void serialize_emptyFile_startsWithHeader()
    {
        PortIni ini;
        ini.parse(QString());
        QVERIFY(ini.serialize().startsWith(QStringLiteral("# bbport settings")));
    }

    void remove_binding_dropsTheLine()
    {
        PortIni ini;
        ini.parse(QStringLiteral("key.cross=G\n"));
        ini.remove(QStringLiteral("key.cross"));
        QVERIFY(ini.serialize().contains(QStringLiteral("key.cross")) == false);
        QVERIFY(ini.contains(QStringLiteral("key.cross")) == false);
    }

    void saveChanges_inGameEdit_isKeptWhenTheLauncherDidNotTouchTheKey()
    {
        QTemporaryDir dir;
        const QString path = dir.filePath(QStringLiteral("bbport.ini"));
        PortIni shown;
        shown.parse(QStringLiteral("upscaler=fsr4\npreset=1\n"));
        QVERIFY(shown.save(path));

        // The in-game menu switches the upscaler while the launcher shows the old value.
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
        file.write("upscaler=fsr3\npreset=1\n");
        file.close();

        PortIni edited = shown;
        edited.setValue(QStringLiteral("preset"), QStringLiteral("2"));
        QVERIFY(PortIni::saveChanges(path, shown, edited));

        PortIni result;
        result.load(path);
        QCOMPARE(result.value(QStringLiteral("upscaler")), QStringLiteral("fsr3"));
        QCOMPARE(result.value(QStringLiteral("preset")), QStringLiteral("2"));
        // Keys the file lacked are written with the launcher's value.
        QVERIFY(result.inFile(QStringLiteral("sharpness")));
    }
};

QTEST_MAIN(TstPortIni)
#include "tst_portIni.moc"
