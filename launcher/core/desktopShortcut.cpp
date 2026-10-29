#include "desktopShortcut.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>

namespace
{
const QString PLAY = QStringLiteral("--play");
// Characters that make an argument need quotes (Desktop Entry Specification, "The Exec key").
const QString RESERVED = QStringLiteral(" \t\n\"'\\><~|&;$*?#()`");
// Characters escaped with a backslash inside the quotes.
const QString ESCAPED_IN_QUOTES = QStringLiteral("\"`$\\");
const QFileDevice::Permissions EXECUTABLE = QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner
                                          | QFileDevice::ReadGroup | QFileDevice::ExeGroup | QFileDevice::ReadOther
                                          | QFileDevice::ExeOther;

QString quoted(const QString& argument)
{
    bool reserved = argument.isEmpty();
    for (const QChar character : argument)
    {
        if (RESERVED.contains(character))
        {
            reserved = true;
            break;
        }
    }
    if (reserved == false)
    {
        return argument;
    }
    QString text = QStringLiteral("\"");
    for (const QChar character : argument)
    {
        if (ESCAPED_IN_QUOTES.contains(character))
        {
            text += u'\\';
        }
        text += character;
    }
    return text + u'"';
}

bool writeFile(const QString& path, const QByteArray& data, QFileDevice::Permissions permissions)
{
    if (QDir().mkpath(QFileInfo(path).absolutePath()) == false) return false;
    // A file copied from the launcher's resources is read-only: replaced, not written over.
    if (QFileInfo::exists(path) && QFileInfo(path).isWritable() == false)
    {
        QFile::remove(path);
    }
    QSaveFile file(path);
    if (file.open(QIODevice::WriteOnly) == false) return false;
    file.write(data);
    if (file.commit() == false) return false;
    return QFile::setPermissions(path, permissions);
}
} // namespace

QStringList DesktopShortcut::playCommand(const QString& appImage, const QString& program)
{
    return { appImage.isEmpty() ? program : appImage, PLAY };
}

QString DesktopShortcut::execLine(const QStringList& command)
{
    QStringList arguments;
    for (const QString& argument : command)
    {
        // The quoting first, then the escapes of a string value (a backslash doubled), then the
        // field codes' %%.
        QString text = quoted(argument);
        text.replace(u'\\', QStringLiteral("\\\\"));
        text.replace(u'%', QStringLiteral("%%"));
        arguments.append(text);
    }
    return arguments.join(u' ');
}

QString DesktopShortcut::entry(const QStringList& command, const QString& icon)
{
    const QString comment = QCoreApplication::translate("DesktopShortcut", "Bloodborne (bbpc) with the launcher's saved settings");
    return QStringLiteral("[Desktop Entry]\n"
                          "Type=Application\n"
                          "Name=Bloodborne\n"
                          "Comment=%1\n"
                          "Exec=%2\n"
                          "Icon=%3\n"
                          "Terminal=false\n"
                          "Categories=Game;\n")
        .arg(comment, execLine(command), icon);
}

QStringList DesktopShortcut::folders(const QString& applicationsDir, const QString& desktopDir, const QString& homeDir)
{
    QStringList list = { applicationsDir };
    const QString desktop = QDir::cleanPath(desktopDir);
    if (desktopDir.isEmpty() == false && desktop != QDir::cleanPath(homeDir) && QFileInfo(desktop).isDir())
    {
        list.append(desktop);
    }
    return list;
}

DesktopShortcut::Result DesktopShortcut::write(const QString& text, const QStringList& folders)
{
    Result result;
    for (const QString& folder : folders)
    {
        const QString path = QDir(folder).filePath(FILE_NAME);
        if (writeFile(path, text.toUtf8(), EXECUTABLE))
        {
            result.written.append(path);
        }
        else if (result.error.isEmpty())
        {
            result.error = QCoreApplication::translate("DesktopShortcut", "Could not write %1").arg(path);
        }
    }
    return result;
}

bool DesktopShortcut::writeIcon(const QByteArray& data, const QString& path)
{
    const QFileDevice::Permissions readable = QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ReadGroup
                                            | QFileDevice::ReadOther;
    return writeFile(path, data, readable);
}
