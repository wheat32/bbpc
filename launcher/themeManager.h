#pragma once

#include <QString>

// ThemeManager - applies a palette and the matching stylesheet (style_dark.qss / style_light.qss)
// to the running application. Call apply() at startup and when the theme setting changes.
namespace ThemeManager
{
    // "system" follows the desktop's color scheme; "dark" and "light" force one.
    void apply(const QString& theme);
}
