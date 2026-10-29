#pragma once

#include <QByteArray>
#include <QString>
#include <QStringList>

// DesktopShortcut - a menu entry and a desktop icon that start the game at once with the saved
// settings, patches and mods (the launcher's --play), without the launcher's window.
namespace DesktopShortcut
{
    inline const QString FILE_NAME = QStringLiteral("bloodborne-bbpc.desktop");
    inline const QString ICON_NAME = QStringLiteral("io.github.wheat32.bbpc.svg");

    // The command that plays: the AppImage when the launcher runs from one (APPIMAGE), else the
    // launcher's own program.
    QStringList playCommand(const QString& appImage, const QString& program);

    // Exec= for a command, its arguments quoted and escaped as the Desktop Entry Specification
    // asks (reserved characters, the string escapes, %% for a literal %).
    QString execLine(const QStringList& command);

    // The desktop entry's text.
    QString entry(const QStringList& command, const QString& icon);

    // The folders the entry goes to: the applications menu, and the desktop folder when it
    // exists and is not the home folder itself (no desktop configured).
    QStringList folders(const QString& applicationsDir, const QString& desktopDir, const QString& homeDir);

    struct Result
    {
        QStringList written;
        // Why a folder could not be written to; empty when every file was written.
        QString error;
    };

    // The entry as an executable FILE_NAME in each folder.
    Result write(const QString& text, const QStringList& folders);

    // The icon at path (its folder created), replacing an older one; false when it cannot be written.
    bool writeIcon(const QByteArray& data, const QString& path);
}
