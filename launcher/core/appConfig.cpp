#include "appConfig.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QSaveFile>
#include <QStandardPaths>

#include "debug.h"
#include "portPaths.h"

namespace
{
QString configRoot()
{
    return QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
}

QJsonObject readObject(const QString& path)
{
    QFile file(path);
    if (file.open(QIODevice::ReadOnly) == false)
    {
        return {};
    }
    return QJsonDocument::fromJson(file.readAll()).object();
}
} // namespace

AppConfig& AppConfig::instance()
{
    static AppConfig config;
    return config;
}

AppConfig::AppConfig()
{
    load();
}

QString AppConfig::configFile()
{
    return configRoot() + QStringLiteral("/bbpc/launcher.json");
}

QString AppConfig::legacyConfigFile()
{
    return configRoot() + QStringLiteral("/bbport-launcher/settings.json");
}

QString AppConfig::defaultGameDir()
{
    if (PortPaths::packaged() || PortPaths::portDir().isEmpty())
    {
        return {};
    }
    // game/CUSA03173 in the checkout (the dump or a symlink to it, ignored by git), else the
    // checkout's sibling, as run.sh looks for it.
    const QString inCheckout = PortPaths::portDir() + QStringLiteral("/game/CUSA03173");
    if (QFileInfo(inCheckout + QStringLiteral("/eboot.bin")).isFile())
    {
        return inCheckout;
    }
    return QDir::cleanPath(PortPaths::portDir() + QStringLiteral("/../CUSA03173"));
}

void AppConfig::load()
{
    LauncherSettings defaults;
    defaults.gameDir = defaultGameDir();

    QString source = configFile();
    if (QFileInfo::exists(source) == false && QFileInfo::exists(legacyConfigFile()))
    {
        source = legacyConfigFile();
        DBG_SETTINGS(QStringLiteral("Importing the original launcher's settings: ") + source);
    }
    m_settings = LauncherSettings::fromJson(readObject(source), defaults);
    DBG_SETTINGS(QStringLiteral("Settings loaded from: ") + source);
    DBG_SETTINGS(QStringLiteral("  game_dir = ") + m_settings.gameDir);
    DBG_SETTINGS(QStringLiteral("  user_dir = ") + m_settings.userDir);
}

bool AppConfig::save() const
{
    const QString path = configFile();
    const QFileDevice::Permissions ownerOnly = QFileDevice::ReadOwner | QFileDevice::WriteOwner;
    QDir().mkpath(QFileInfo(path).absolutePath());
    // The file exists with its permissions before anything is written to it (QSaveFile keeps the
    // permissions of the file it replaces).
    if (QFileInfo::exists(path) == false)
    {
        QFile empty(path);
        if (empty.open(QIODevice::WriteOnly))
        {
            empty.setPermissions(ownerOnly);
        }
    }
    QFile::setPermissions(path, ownerOnly);
    QSaveFile file(path);
    if (file.open(QIODevice::WriteOnly) == false)
    {
        DBG_SETTINGS(QStringLiteral("Could not write ") + path + QStringLiteral(": ") + file.errorString());
        return false;
    }
    file.write(QJsonDocument(m_settings.toJson()).toJson(QJsonDocument::Indented));
    const bool written = file.commit();
    QFile::setPermissions(path, ownerOnly);
    return written;
}
