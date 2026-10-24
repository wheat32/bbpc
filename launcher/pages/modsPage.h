#pragma once

#include <QList>
#include <QString>

#include "core/profiles.h"
#include "launcherPage.h"

class SettingRow;
class SwitchRow;

// ModsPage - loose-file mods (enabled one by one, in load order) and shadPS4-format XML patches
// from the patches folder. The selections go to mods.json and patches.json in the data folder,
// which run.sh applies at start.
class ModsPage : public LauncherPage
{
    Q_OBJECT

public:
    explicit ModsPage(const LauncherSettings& settings, const PortIni& ini, QWidget* parent = nullptr);

    void store(LauncherSettings& settings, PortIni& ini) const override;

private:
    [[nodiscard]] QString modsDir() const;
    [[nodiscard]] QString patchesDir() const;
    void refreshMods();
    void showMods();
    void moveMod(int index, int direction);
    void saveMods() const;
    void refreshPatches();
    void savePatches() const;

    QString m_modsDir;
    QString m_patchesDir;
    QList<Profiles::Mod> m_mods;
    // mods.json is written only once the list it would replace is known.
    bool m_modsLoaded = false;
    QList<Profiles::Patch> m_patches;

    SettingsGroup* m_modsGroup = nullptr;
    SwitchRow* m_modsEnabledRow = nullptr;
    SettingRow* m_modsFolderRow = nullptr;
    QList<QWidget*> m_modRows;
    SettingsGroup* m_patchesGroup = nullptr;
    SettingRow* m_patchesFolderRow = nullptr;
    QList<SwitchRow*> m_patchRows;
};
