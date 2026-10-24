#pragma once

#include "core/launcherSettings.h"
#include "core/portIni.h"
#include "widgets/settingsGroup.h"

// LauncherPage - a page of settings. Each page builds its controls from the settings and
// bbport.ini, and writes them back in store(); the window calls it before it saves or starts the
// game.
class LauncherPage : public SettingsPage
{
    Q_OBJECT

public:
    using SettingsPage::SettingsPage;

    virtual void store(LauncherSettings& settings, PortIni& ini) const = 0;

signals:
    // A setting changed: the window saves soon.
    void changed();
    // A short message for the toast at the bottom of the window.
    void toast(const QString& message);
};
