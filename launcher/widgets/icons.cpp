#include "icons.h"

#include <QApplication>
#include <QStyle>
#include <QToolButton>

namespace
{
constexpr int ICON_SIZE = 18;

struct IconSource
{
    const char* themeName;
    QStyle::StandardPixmap fallback;
};

IconSource source(Icons::Kind kind)
{
    switch (kind)
    {
        case Icons::Kind::FolderOpen:
            return { "folder-open", QStyle::SP_DirOpenIcon };
        case Icons::Kind::FileManager:
            return { "system-file-manager", QStyle::SP_DirLinkIcon };
        case Icons::Kind::Refresh:
            return { "view-refresh", QStyle::SP_BrowserReload };
        case Icons::Kind::Undo:
            return { "edit-undo", QStyle::SP_DialogResetButton };
        case Icons::Kind::Up:
            return { "go-up", QStyle::SP_ArrowUp };
        case Icons::Kind::Down:
            return { "go-down", QStyle::SP_ArrowDown };
        case Icons::Kind::Keyboard:
            return { "input-keyboard", QStyle::SP_ComputerIcon };
        case Icons::Kind::Gamepad:
            return { "input-gaming", QStyle::SP_MediaPlay };
        case Icons::Kind::Ok:
            return { "emblem-ok-symbolic", QStyle::SP_DialogApplyButton };
        case Icons::Kind::Warning:
            return { "dialog-warning", QStyle::SP_MessageBoxWarning };
        case Icons::Kind::New:
            return { "document-new", QStyle::SP_FileDialogNewFolder };
        case Icons::Kind::Document:
            return { "document-open", QStyle::SP_FileIcon };
        case Icons::Kind::Copy:
            return { "edit-copy", QStyle::SP_FileDialogContentsView };
        case Icons::Kind::Save:
            return { "document-save", QStyle::SP_DialogSaveButton };
        case Icons::Kind::Add:
            return { "list-add", QStyle::SP_FileDialogNewFolder };
        case Icons::Kind::Trophy:
            return { "starred", QStyle::SP_DialogYesButton };
    }
    return { "", QStyle::SP_FileIcon };
}
} // namespace

QIcon Icons::get(Kind kind)
{
    const IconSource icon = source(kind);
    return QIcon::fromTheme(QString::fromLatin1(icon.themeName), QApplication::style()->standardIcon(icon.fallback));
}

QToolButton* Icons::button(Kind kind, const QString& toolTip, QWidget* parent)
{
    QToolButton* button = new QToolButton(parent);
    button->setIcon(get(kind));
    button->setIconSize(QSize(ICON_SIZE, ICON_SIZE));
    button->setToolTip(toolTip);
    button->setAutoRaise(true);
    button->setCursor(Qt::PointingHandCursor);
    return button;
}
