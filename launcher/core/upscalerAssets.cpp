#include "upscalerAssets.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <algorithm>

#include "outputSizes.h"
// The game's own table (header only, the standard library alone), so both name DLSS alike.
#include "../../gpu/upscale/dlss_generation.h"

namespace
{
constexpr int ULTRA_PERFORMANCE_PRESET = 4;
constexpr int FIRST_PASS = 1;
constexpr int PASS_COUNT = 12;
constexpr qint64 MIN_SPIRV_SIZE = 20;
constexpr qint64 SPIRV_WORD_SIZE = 4;
constexpr qint64 INITIALIZER_SIZE = 131072;
constexpr int ELF_MAGIC_SIZE = 4;
const QByteArray ELF_MAGIC = QByteArrayLiteral("\x7f" "ELF");

QString missing(const QString& file)
{
    return QCoreApplication::translate("UpscalerAssets", "Missing or damaged file %1").arg(file);
}

// The numeric parts of a version ("310.4.0" -> 310, 4, 0); other parts count as 0.
QList<int> versionParts(const QString& version)
{
    QList<int> parts;
    for (const QString& part : version.split(u'.'))
    {
        bool ok = false;
        const int number = part.toInt(&ok);
        parts.append(ok ? number : 0);
    }
    return parts;
}
} // namespace

QString UpscalerAssets::fsr411Dir(const QString& portDir, const QString& dataDir)
{
    const QString explicitDir = qEnvironmentVariable("BB_FSR411_DIR");
    if (explicitDir.isEmpty() == false)
    {
        return explicitDir;
    }
    const QString beside = portDir + QStringLiteral("/fsr4_411");
    return QFileInfo(beside).isDir() ? beside : dataDir + QStringLiteral("/fsr4_411");
}

QString UpscalerAssets::fsr411Problem(const QString& directory, const QString& output, int preset)
{
    const QSize size = OutputSizes::parse(output).value_or(OutputSizes::DEFAULT_SIZE);
    const bool large = size.width() > OutputSizes::DEFAULT_SIZE.width() || size.height() > OutputSizes::DEFAULT_SIZE.height();
    const QString model = (large ? QStringLiteral("t2160") : QStringLiteral("t1080"))
                        + (preset == ULTRA_PERFORMANCE_PRESET ? QStringLiteral("_m1") : QStringLiteral("_m0"));
    QStringList passes = { QStringLiteral("spd"), QStringLiteral("prepass"), QStringLiteral("pass0_post") };
    for (int i = FIRST_PASS; i <= PASS_COUNT; ++i)
    {
        passes.append(QStringLiteral("pass%1").arg(i));
        passes.append(QStringLiteral("pass%1_post").arg(i));
    }
    passes.append(QStringLiteral("postpass"));
    passes.append(QStringLiteral("rcas"));
    for (const QString& pass : passes)
    {
        const QString relative = model + u'/' + pass + QStringLiteral(".spv");
        const QFileInfo file(directory + u'/' + relative);
        if (file.isFile() == false || file.size() < MIN_SPIRV_SIZE || file.size() % SPIRV_WORD_SIZE != 0)
        {
            return missing(relative);
        }
    }
    const QString initializer = model + QStringLiteral("/initializer.bin");
    const QFileInfo file(directory + u'/' + initializer);
    if (file.isFile() == false || file.size() != INITIALIZER_SIZE)
    {
        return missing(initializer);
    }
    return {};
}

QString UpscalerAssets::ngxVersion(const QFileInfo& library)
{
    return library.fileName().mid(NGX_PREFIX.size());
}

QFileInfoList UpscalerAssets::ngxLibraries(const QString& directory)
{
    QFileInfoList libraries;
    const QFileInfoList entries = QDir(directory).entryInfoList(QDir::Files | QDir::NoDotAndDotDot);
    for (const QFileInfo& entry : entries)
    {
        if (entry.fileName().startsWith(NGX_PREFIX))
        {
            libraries.append(entry);
        }
    }
    std::stable_sort(libraries.begin(), libraries.end(), [](const QFileInfo& a, const QFileInfo& b)
    {
        return versionParts(ngxVersion(a)) < versionParts(ngxVersion(b));
    });
    return libraries;
}

QFileInfo UpscalerAssets::ngxInUse(const QString& ownDirectory, const QString& packageDirectory)
{
    for (const QString& directory : { ownDirectory, packageDirectory })
    {
        const QFileInfoList libraries = ngxLibraries(directory);
        if (libraries.isEmpty() == false)
        {
            return libraries.last();
        }
    }
    return {};
}

QString UpscalerAssets::dlssGeneration(const QString& libraryVersion, int preset)
{
    const QByteArray version = libraryVersion.toLatin1();
    const std::string_view generation = DlssGeneration::Of({version.constData(), size_t(version.size())}, preset);
    return QString::fromLatin1(generation.data(), qsizetype(generation.size()));
}

bool UpscalerAssets::isElf(const QString& path)
{
    QFile file(path);
    if (file.open(QIODevice::ReadOnly) == false) return false;
    return file.read(ELF_MAGIC_SIZE) == ELF_MAGIC;
}
