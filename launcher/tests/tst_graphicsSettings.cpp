#include <QtTest/QtTest>

#include "graphicsSettings.h"
#include "portIni.h"

namespace
{
const Capabilities::Gpu AMD_GPU { 0, QStringLiteral("0x1002"), QStringLiteral("discrete"), QStringLiteral("AMD Radeon RX 7800 XT"), true };
const Capabilities::Gpu NVIDIA_GPU { 1, QStringLiteral("0x10de"), QStringLiteral("discrete"), QStringLiteral("NVIDIA GeForce RTX 4070"), false };
} // namespace

class TstGraphicsSettings : public QObject
{
    Q_OBJECT

private slots:
    void frameGenerationChoice_dlss_carriesTheMultiplier()
    {
        QCOMPARE(GraphicsSettings::frameGenerationChoice(QStringLiteral("dlss"), QStringLiteral("3")), QStringLiteral("dlss3"));
        QCOMPARE(GraphicsSettings::frameGenerationChoice(QStringLiteral("dlss"), QStringLiteral("9")), QStringLiteral("dlss4"));
        QCOMPARE(GraphicsSettings::frameGenerationChoice(QStringLiteral("dlss"), QStringLiteral("x")), QStringLiteral("dlss2"));
    }

    void frameGenerationChoice_olderValues_areReadAsTheGameReadsThem()
    {
        QCOMPARE(GraphicsSettings::frameGenerationChoice(QStringLiteral("1"), QStringLiteral("2")), QStringLiteral("fsr3"));
        QCOMPARE(GraphicsSettings::frameGenerationChoice(QStringLiteral("0"), QStringLiteral("2")), QStringLiteral("off"));
        QCOMPARE(GraphicsSettings::frameGenerationChoice(QStringLiteral("fsr3"), QStringLiteral("4")), QStringLiteral("fsr3"));
        QCOMPARE(GraphicsSettings::frameGenerationChoice(QStringLiteral("fsr_amd"), QStringLiteral("2")), QStringLiteral("off"));
    }

    void frameGenerationKeys_dlss_setsTheMultiplier()
    {
        const GraphicsSettings::FrameGenerationKeys keys = GraphicsSettings::frameGenerationKeys(QStringLiteral("dlss4"), QStringLiteral("2"));
        QCOMPARE(keys.frameGeneration, QStringLiteral("dlss"));
        QCOMPARE(keys.multiplier, QStringLiteral("4"));
    }

    void frameGenerationKeys_others_keepTheMultiplier()
    {
        const GraphicsSettings::FrameGenerationKeys fsr3 = GraphicsSettings::frameGenerationKeys(QStringLiteral("fsr3"), QStringLiteral("3"));
        QCOMPARE(fsr3.frameGeneration, QStringLiteral("fsr3"));
        QCOMPARE(fsr3.multiplier, QStringLiteral("3"));
        QCOMPARE(GraphicsSettings::frameGenerationKeys(QStringLiteral("junk"), QStringLiteral("2")).frameGeneration, QStringLiteral("off"));
    }

    void offersDlssFrameGeneration_dependsOnTheGpus()
    {
        QVERIFY(GraphicsSettings::offersDlssFrameGeneration({}, QStringLiteral("off")));
        QVERIFY(GraphicsSettings::offersDlssFrameGeneration({ AMD_GPU, NVIDIA_GPU }, QStringLiteral("off")));
        QVERIFY(GraphicsSettings::offersDlssFrameGeneration({ AMD_GPU }, QStringLiteral("off")) == false);
        // The saved choice stays listed.
        QVERIFY(GraphicsSettings::offersDlssFrameGeneration({ AMD_GPU }, QStringLiteral("dlss2")));
    }

    void lowLatency_dlssFrameGeneration_keepsItOn()
    {
        QCOMPARE(GraphicsSettings::lowLatency(QStringLiteral("off"), QStringLiteral("dlss2")), QStringLiteral("on"));
        QCOMPARE(GraphicsSettings::lowLatency(QStringLiteral("boost"), QStringLiteral("dlss2")), QStringLiteral("boost"));
        QCOMPARE(GraphicsSettings::lowLatency(QStringLiteral("off"), QStringLiteral("fsr3")), QStringLiteral("off"));
    }

    void lowLatency_numbersAndUnknownValues_becomeChoices()
    {
        QCOMPARE(GraphicsSettings::lowLatency(QStringLiteral("2"), QStringLiteral("off")), QStringLiteral("boost"));
        QCOMPARE(GraphicsSettings::lowLatency(QStringLiteral("0"), QStringLiteral("off")), QStringLiteral("off"));
        QCOMPARE(GraphicsSettings::lowLatency(QStringLiteral("fast"), QStringLiteral("off")), QStringLiteral("on"));
    }

    void antiAliasing_olderSettingsWithTheGamesAaOff_stayOff()
    {
        PortIni ini;
        ini.parse(QStringLiteral("effect_game_aa=0\n"));
        QCOMPARE(GraphicsSettings::antiAliasing(ini), QStringLiteral("off"));
        ini.parse(QStringLiteral("effect_game_aa=1\n"));
        QCOMPARE(GraphicsSettings::antiAliasing(ini), QStringLiteral("auto"));
    }

    void antiAliasing_savedChoice_winsOverTheOldSwitch()
    {
        PortIni ini;
        ini.parse(QStringLiteral("effect_game_aa=0\nanti_aliasing=smaa\n"));
        QCOMPARE(GraphicsSettings::antiAliasing(ini), QStringLiteral("smaa"));
        ini.parse(QStringLiteral("anti_aliasing=sharp\n"));
        QCOMPARE(GraphicsSettings::antiAliasing(ini), QStringLiteral("auto"));
    }
};

QTEST_MAIN(TstGraphicsSettings)
#include "tst_graphicsSettings.moc"
