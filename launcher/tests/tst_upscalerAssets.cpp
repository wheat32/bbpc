#include <QtTest/QtTest>

#include "upscalerAssets.h"

namespace
{
constexpr int SPIRV_SIZE = 20;
constexpr int SHORT_INITIALIZER = 8;
constexpr int INITIALIZER_SIZE = 131072;
constexpr int FIRST_PASS = 1;
constexpr int PASS_COUNT = 12;
// Choices::presets(): the render presets by index.
constexpr int PRESET_NATIVE_AA = 0;
constexpr int PRESET_QUALITY = 1;
constexpr int PRESET_PERFORMANCE = 3;
constexpr int PRESET_ULTRA_PERFORMANCE = 4;

void writeFile(const QString& path, int size)
{
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(QByteArray(size, '\0'));
}
} // namespace

// The FSR 4.1.1 model check of the original launcher's tests/test_upscaler_assets.py.
class TstUpscalerAssets : public QObject
{
    Q_OBJECT

private slots:
    void fsr411Problem_modelTier_dependsOnTheOutputNotTheRenderPreset()
    {
        QTemporaryDir dir;
        QVERIFY(UpscalerAssets::fsr411Problem(dir.path(), QStringLiteral("3840x2160"), PRESET_PERFORMANCE)
                    .contains(QStringLiteral("t2160_m0/spd.spv")));
        QVERIFY(UpscalerAssets::fsr411Problem(dir.path(), QStringLiteral("2560x1440"), PRESET_NATIVE_AA)
                    .contains(QStringLiteral("t2160_m0/spd.spv")));
        QVERIFY(UpscalerAssets::fsr411Problem(dir.path(), QStringLiteral("1280x720"), PRESET_ULTRA_PERFORMANCE)
                    .contains(QStringLiteral("t1080_m1/spd.spv")));
    }

    void fsr411Problem_partialModel_isNotAvailable()
    {
        QTemporaryDir dir;
        writeFile(dir.filePath(QStringLiteral("t2160_m0/spd.spv")), SPIRV_SIZE);
        QVERIFY(UpscalerAssets::fsr411Problem(dir.path(), QStringLiteral("3840x2160"), PRESET_NATIVE_AA)
                    .contains(QStringLiteral("prepass.spv")));
    }

    void fsr411Problem_completeModel_needsAFullInitializer()
    {
        QTemporaryDir dir;
        QStringList names = { QStringLiteral("spd"), QStringLiteral("prepass"), QStringLiteral("pass0_post"),
                              QStringLiteral("postpass"), QStringLiteral("rcas") };
        for (int i = FIRST_PASS; i <= PASS_COUNT; ++i)
        {
            names.append(QStringLiteral("pass%1").arg(i));
            names.append(QStringLiteral("pass%1_post").arg(i));
        }
        for (const QString& name : names)
        {
            writeFile(dir.filePath(QStringLiteral("t1080_m0/") + name + QStringLiteral(".spv")), SPIRV_SIZE);
        }
        writeFile(dir.filePath(QStringLiteral("t1080_m0/initializer.bin")), SHORT_INITIALIZER);
        QVERIFY(UpscalerAssets::fsr411Problem(dir.path(), QStringLiteral("1920x1080"), PRESET_QUALITY)
                    .contains(QStringLiteral("initializer.bin")));
        writeFile(dir.filePath(QStringLiteral("t1080_m0/initializer.bin")), INITIALIZER_SIZE);
        QVERIFY(UpscalerAssets::fsr411Problem(dir.path(), QStringLiteral("1920x1080"), PRESET_QUALITY).isEmpty());
    }

    void ngxLibraries_versions_sortNumerically()
    {
        QTemporaryDir dir;
        for (const QString& version : { QStringLiteral("310.10.0"), QStringLiteral("310.4.0"), QStringLiteral("39.0.0") })
        {
            writeFile(dir.filePath(UpscalerAssets::NGX_PREFIX + version), SPIRV_SIZE);
        }
        writeFile(dir.filePath(QStringLiteral("other.so")), SPIRV_SIZE);
        const QFileInfoList libraries = UpscalerAssets::ngxLibraries(dir.path());
        QCOMPARE(libraries.size(), 3);
        QCOMPARE(UpscalerAssets::ngxVersion(libraries.first()), QStringLiteral("39.0.0"));
        QCOMPARE(UpscalerAssets::ngxVersion(libraries.last()), QStringLiteral("310.10.0"));
    }

    void ngxInUse_ownLibrary_winsOverANewerPackageOne()
    {
        QTemporaryDir own;
        QTemporaryDir package;
        writeFile(own.filePath(UpscalerAssets::NGX_PREFIX + QStringLiteral("310.4.0")), SPIRV_SIZE);
        writeFile(package.filePath(UpscalerAssets::NGX_PREFIX + QStringLiteral("310.9.16")), SPIRV_SIZE);
        QCOMPARE(UpscalerAssets::ngxVersion(UpscalerAssets::ngxInUse(own.path(), package.path())),
                 QStringLiteral("310.4.0"));
    }

    void ngxInUse_noOwnLibrary_isThePackagesNewest()
    {
        QTemporaryDir own;
        QTemporaryDir package;
        for (const QString& version : { QStringLiteral("310.10.0"), QStringLiteral("310.9.16") })
        {
            writeFile(package.filePath(UpscalerAssets::NGX_PREFIX + version), SPIRV_SIZE);
        }
        QCOMPARE(UpscalerAssets::ngxVersion(UpscalerAssets::ngxInUse(own.path(), package.path())),
                 QStringLiteral("310.10.0"));
    }

    void ngxInUse_noLibrary_isEmpty()
    {
        QTemporaryDir own;
        QTemporaryDir package;
        QVERIFY(UpscalerAssets::ngxInUse(own.path(), package.path()).fileName().isEmpty());
    }

    void dlssGeneration_presets_followNvidiasDefaults()
    {
        QCOMPARE(UpscalerAssets::dlssGeneration(QStringLiteral("310.9.16"), 1), QStringLiteral("4"));
        QCOMPARE(UpscalerAssets::dlssGeneration(QStringLiteral("310.9.16"), 3), QStringLiteral("4.5"));
        QCOMPARE(UpscalerAssets::dlssGeneration(QStringLiteral("3.7.20"), 1), QStringLiteral("3"));
        QVERIFY(UpscalerAssets::dlssGeneration(QStringLiteral("320.1.0"), 1).isEmpty());
    }

    void isElf_header_isRecognized()
    {
        QTemporaryDir dir;
        const QString elf = dir.filePath(QStringLiteral("lib.so"));
        QFile file(elf);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("\x7f" "ELF\x02\x01");
        file.close();
        QVERIFY(UpscalerAssets::isElf(elf));
        writeFile(dir.filePath(QStringLiteral("text.so")), SPIRV_SIZE);
        QVERIFY(UpscalerAssets::isElf(dir.filePath(QStringLiteral("text.so"))) == false);
    }
};

QTEST_MAIN(TstUpscalerAssets)
#include "tst_upscalerAssets.moc"
