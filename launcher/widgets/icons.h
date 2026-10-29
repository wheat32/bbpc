#pragma once

#include <QIcon>
#include <QString>

class QToolButton;
class QWidget;

// Icons - the desktop's icon theme where it has the icon, Qt's own standard icons otherwise (an
// AppImage on a desktop without an icon theme still gets every icon).
namespace Icons
{
    enum class Kind
    {
        FolderOpen,
        FileManager,
        Refresh,
        Undo,
        Up,
        Down,
        Keyboard,
        Gamepad,
        Ok,
        Warning,
        New,
        Document,
        Copy,
        Save,
        Add,
        Trophy,
    };

    QIcon get(Kind kind);

    // A flat icon button with a tooltip, as the rows' actions.
    QToolButton* button(Kind kind, const QString& toolTip, QWidget* parent);
}
