#include "portPaths.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

namespace
{
const QString DATA_FOLDER_NAME = QStringLiteral("bbpc");

QString cleanAbsolute(const QString& path)
{
    return QDir::cleanPath(QDir(path).absolutePath());
}

QString applicationDir()
{
    return QCoreApplication::applicationDirPath();
}
} // namespace

bool PortPaths::isPortDir(const QString& folder)
{
    return QFileInfo(folder + QStringLiteral("/run.sh")).isFile()
        && QFileInfo(folder + QStringLiteral("/scripts")).isDir();
}

QString PortPaths::findPortDir(const QString& applicationDir)
{
    // Installed: <prefix>/bin/bbpc and <prefix>/share/bbpc. From source: <repo>/<build>/launcher/bbpc.
    const QStringList candidates = {
        applicationDir + QStringLiteral("/../share/bbpc"),
        applicationDir + QStringLiteral("/../.."),
        applicationDir + QStringLiteral("/.."),
    };
    for (const QString& candidate : candidates)
    {
        if (isPortDir(candidate))
        {
            return cleanAbsolute(candidate);
        }
    }
    return {};
}

QString PortPaths::portDir()
{
    static const QString dir = []()
    {
        const QString explicitDir = qEnvironmentVariable("BBPC_PORT_DIR");
        if (explicitDir.isEmpty() == false)
        {
            return cleanAbsolute(explicitDir);
        }
        return findPortDir(applicationDir());
    }();
    return dir;
}

bool PortPaths::packaged()
{
    if (qEnvironmentVariableIsEmpty("BB_PREBUILT") == false) return true;
    const QString dir = portDir();
    return QFileInfo(dir + QStringLiteral("/bin/bb-probe")).isFile()
        && QFileInfo::exists(dir + QStringLiteral("/CMakeLists.txt")) == false;
}

QString PortPaths::buildDir()
{
    if (packaged())
    {
        return {};
    }
    // The launcher's own build folder when it holds the game too (any preset), else the release
    // preset's out/ (the launcher-only preset builds no game).
    const QString ownBuild = cleanAbsolute(applicationDir() + QStringLiteral("/.."));
    if (QFileInfo(ownBuild + QStringLiteral("/bb-probe")).isFile())
    {
        return ownBuild;
    }
    return portDir() + QStringLiteral("/out");
}

QString PortPaths::binariesDir()
{
    if (packaged())
    {
        return portDir() + QStringLiteral("/bin");
    }
    return buildDir();
}

QString PortPaths::dataDir()
{
    const QString explicitDir = qEnvironmentVariable("BB_DATA_DIR");
    if (explicitDir.isEmpty() == false)
    {
        return cleanAbsolute(explicitDir);
    }
    if (packaged())
    {
        return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + u'/' + DATA_FOLDER_NAME;
    }
    return portDir();
}

QString PortPaths::iniPath()
{
    const QString explicitPath = qEnvironmentVariable("BB_CONFIG");
    if (explicitPath.isEmpty() == false)
    {
        return explicitPath;
    }
    return dataDir() + QStringLiteral("/bbport.ini");
}

QString PortPaths::capabilitiesTool()
{
    return binariesDir() + QStringLiteral("/bb-gpu-capabilities");
}

QString PortPaths::fsr4capDir()
{
    return portDir() + QStringLiteral("/tools/fsr4cap");
}

QString PortPaths::python()
{
    const QString explicitPython = qEnvironmentVariable("PYTHON");
    return explicitPython.isEmpty() ? QStringLiteral("python3") : explicitPython;
}

bool PortPaths::hasOnlineModule(const QString& netModule, const QString& binariesDir)
{
    if (netModule.isEmpty() == false)
    {
        return QFileInfo(netModule).isFile();
    }
    return QFileInfo(binariesDir + QStringLiteral("/gpu/libbbnet20.so")).isFile();
}

bool PortPaths::hasOnlineModule()
{
    return hasOnlineModule(qEnvironmentVariable("BB_NET_MODULE"), binariesDir());
}

QString PortPaths::upstreamVersion(const QByteArray& versionJson)
{
    const QJsonObject json = QJsonDocument::fromJson(versionJson).object();
    return json.value(QStringLiteral("upstream_version")).toString().trimmed();
}

QString PortPaths::portVersion()
{
    QFile file(QStringLiteral(":/version.json"));
    if (file.open(QIODevice::ReadOnly) == false)
    {
        return {};
    }
    return upstreamVersion(file.readAll());
}
