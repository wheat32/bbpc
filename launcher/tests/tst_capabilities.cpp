#include <QtTest/QtTest>

#include "capabilities.h"

// bb-gpu-capabilities' output as tools/gpu_capabilities.c prints it.
class TstCapabilities : public QObject
{
    Q_OBJECT

private slots:
    void parseGpus_vulkanOrder_isKept()
    {
        const QList<Capabilities::Gpu> gpus = Capabilities::parseGpus(QStringLiteral(
            "0\t0x8086\tintegrated\tIntel(R) UHD Graphics 630\t0\n"
            "1\t0x10de\tdiscrete\tNVIDIA GeForce RTX 4060 Laptop GPU\t1\n"
            "2\t0x10005\tcpu\tllvmpipe (LLVM 21.1.8, 256 bits)\t0\n"
            "warning: something else\n"));
        QCOMPARE(gpus.size(), 3);
        QCOMPARE(gpus.at(1).index, 1);
        QCOMPARE(gpus.at(1).name, QStringLiteral("NVIDIA GeForce RTX 4060 Laptop GPU"));
        QVERIFY(gpus.at(1).best);
        QCOMPARE(gpus.at(2).kind, QStringLiteral("cpu"));
    }

    void parseDisplays_sharedNames_useTheNumber()
    {
        const QList<Capabilities::Display> displays = Capabilities::parseDisplays(QStringLiteral(
            "DELL U2720Q\t3840x2160\t1\nDELL U2720Q\t3840x2160\t0\nSteam Deck\t1280x800\t0\n"));
        QCOMPARE(displays.size(), 3);
        QCOMPARE(displays.at(0).value, QStringLiteral("1"));
        QCOMPARE(displays.at(1).value, QStringLiteral("2"));
        QCOMPARE(displays.at(2).value, QStringLiteral("Steam Deck"));
        QVERIFY(displays.at(0).primary);
        QCOMPARE(displays.at(2).size, std::optional<QSize>(QSize(1280, 800)));
    }

    void parseGamepads_guidAndName()
    {
        const QList<Capabilities::Gamepad> gamepads = Capabilities::parseGamepads(QStringLiteral("030000005e04\tXbox Wireless Controller\n"));
        QCOMPARE(gamepads.size(), 1);
        QCOMPARE(gamepads.first().guid, QStringLiteral("030000005e04"));
        QCOMPARE(gamepads.first().name, QStringLiteral("Xbox Wireless Controller"));
    }

    void parseInput_kind_mustMatch()
    {
        QCOMPARE(Capabilities::parseInput(QStringLiteral("key Left Shift+Mouse Left\n"), QStringLiteral("key")),
                 std::optional<QString>(QStringLiteral("Left Shift+Mouse Left")));
        QVERIFY(Capabilities::parseInput(QStringLiteral("pad a"), QStringLiteral("key")).has_value() == false);
        QVERIFY(Capabilities::parseInput(QString(), QStringLiteral("pad")).has_value() == false);
    }
};

QTEST_MAIN(TstCapabilities)
#include "tst_capabilities.moc"
