#pragma once

#include <QString>

#include "launcherSettings.h"

// AppConfig - the launcher's settings, persisted to ~/.config/bbpc/launcher.json. The first run
// imports the original project's GTK launcher settings (~/.config/bbport-launcher/settings.json),
// which use the same keys. The file holds the shadNet password and The Hunter's Dream's token,
// so it is readable by its owner only.
class AppConfig
{
public:
    static AppConfig& instance();

    LauncherSettings& settings()
    {
        return m_settings;
    }

    // Writes the settings; false when the file could not be written.
    bool save() const;

    [[nodiscard]] static QString configFile();
    [[nodiscard]] static QString legacyConfigFile();

    // The game folder the settings start with: from source game/CUSA03173 in the checkout, else
    // its sibling CUSA03173 (as run.sh looks for it); none when packaged.
    [[nodiscard]] static QString defaultGameDir();

private:
    AppConfig();
    void load();

    LauncherSettings m_settings;
};
