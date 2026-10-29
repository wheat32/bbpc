#pragma once

#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QLabel>
#include <QProcess>
#include <QString>
#include <QUrl>

#include "core/hostEnvironment.h"
#include "icons.h"

namespace UiHelpers
{
    inline constexpr int STATUS_ICON_SIZE = 18;

    // Opens path in the file manager, creating it first (a new saves folder opens too).
    inline void openFolder(const QString& path)
    {
        if (QDir().mkpath(path) == false) return;
        QDesktopServices::openUrl(QUrl::fromLocalFile(path));
    }

    // Opens url in the browser with xdg-open in the host's environment (HostEnvironment), so the
    // browser never loads the AppImage's libraries: QDesktopServices goes through the desktop
    // portal, which on some Wayland desktops reports success and opens nothing. False when
    // xdg-open could not be started.
    inline bool openInBrowser(const QString& url)
    {
        QProcess process;
        process.setProgram(QStringLiteral("xdg-open"));
        process.setArguments({ url });
        process.setProcessEnvironment(HostEnvironment::host());
        return process.startDetached();
    }

    // A folder chooser that starts in current when it exists; an empty string when canceled.
    inline QString chooseFolder(QWidget* parent, const QString& title, const QString& current)
    {
        const QString start = QFileInfo(current).isDir() ? current : QDir::homePath();
        return QFileDialog::getExistingDirectory(parent, title, start);
    }

    // A small status icon for a row's left side.
    inline QLabel* statusIcon(QWidget* parent)
    {
        QLabel* label = new QLabel(parent);
        label->setFixedSize(STATUS_ICON_SIZE, STATUS_ICON_SIZE);
        return label;
    }

    inline void setStatusIcon(QLabel* label, Icons::Kind kind, const QString& toolTip)
    {
        label->setPixmap(Icons::get(kind).pixmap(STATUS_ICON_SIZE, STATUS_ICON_SIZE));
        label->setToolTip(toolTip);
    }
}
