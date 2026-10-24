#include "themeManager.h"

#include <QApplication>
#include <QFile>
#include <QPalette>
#include <QStyleHints>

namespace
{
struct Colors
{
    QColor window;
    QColor surface;
    QColor button;
    QColor border;
    QColor accent;
    QColor text;
    QColor secondary;
};

const Colors DARK = {
    QColor(0x14, 0x11, 0x16), QColor(0x1d, 0x1a, 0x20), QColor(0x27, 0x22, 0x2b), QColor(0x3a, 0x33, 0x40),
    QColor(0xb0, 0x41, 0x3e), QColor(0xec, 0xe6, 0xea), QColor(0xa1, 0x96, 0x9e),
};

const Colors LIGHT = {
    QColor(0xf3, 0xef, 0xec), QColor(0xff, 0xff, 0xff), QColor(0xf8, 0xf5, 0xf3), QColor(0xd8, 0xcf, 0xca),
    QColor(0x9c, 0x2f, 0x2b), QColor(0x23, 0x1d, 0x21), QColor(0x6c, 0x60, 0x68),
};

void applyColors(const Colors& colors, const QString& styleSheet)
{
    QPalette palette;
    palette.setColor(QPalette::Window, colors.window);
    palette.setColor(QPalette::WindowText, colors.text);
    palette.setColor(QPalette::Base, colors.surface);
    palette.setColor(QPalette::AlternateBase, colors.window);
    palette.setColor(QPalette::ToolTipBase, colors.button);
    palette.setColor(QPalette::ToolTipText, colors.text);
    palette.setColor(QPalette::Text, colors.text);
    palette.setColor(QPalette::BrightText, Qt::white);
    palette.setColor(QPalette::Button, colors.button);
    palette.setColor(QPalette::ButtonText, colors.text);
    palette.setColor(QPalette::Link, colors.accent);
    palette.setColor(QPalette::Highlight, colors.accent);
    palette.setColor(QPalette::HighlightedText, Qt::white);
    palette.setColor(QPalette::PlaceholderText, colors.secondary);
    palette.setColor(QPalette::Mid, colors.border);
    palette.setColor(QPalette::Dark, colors.border);
    palette.setColor(QPalette::Midlight, colors.button);
    palette.setColor(QPalette::Disabled, QPalette::Text, colors.secondary);
    palette.setColor(QPalette::Disabled, QPalette::WindowText, colors.secondary);
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, colors.secondary);
    QApplication::setPalette(palette);

    QFile file(styleSheet);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qApp->setStyleSheet(QString::fromUtf8(file.readAll()));
    }
}

bool systemIsDark()
{
    return QGuiApplication::styleHints()->colorScheme() != Qt::ColorScheme::Light;
}
} // namespace

void ThemeManager::apply(const QString& theme)
{
    bool dark = systemIsDark();
    if (theme == QStringLiteral("dark"))
    {
        dark = true;
    }
    else if (theme == QStringLiteral("light"))
    {
        dark = false;
    }
    if (dark)
    {
        applyColors(DARK, QStringLiteral(":/style_dark.qss"));
    }
    else
    {
        applyColors(LIGHT, QStringLiteral(":/style_light.qss"));
    }
}
