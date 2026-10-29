#pragma once

#include <QList>
#include <QString>

#include "core/trophies.h"
#include "launcherPage.h"

class EntryRow;
class SettingRow;

// TrophiesPage - the trophies earned in the saves folder (Trophies: what the port keeps there) and
// the player's trophy key, with which the game decrypts the trophy names and icons.
class TrophiesPage : public LauncherPage
{
    Q_OBJECT

public:
    explicit TrophiesPage(const LauncherSettings& settings, const PortIni& ini, QWidget* parent = nullptr);

    void store(LauncherSettings& settings, PortIni& ini) const override;

public slots:
    void setUserDir(const QString& userDir);
    // The PS4 system language the game sees (its trophy texts are read in it).
    void setLanguage(const QString& language);
    // Reads the trophies again (after the game ran, when the page is shown).
    void refresh();

private:
    void showKeyState();
    SettingRow* trophyRow(const Trophies::Trophy& trophy, bool named);
    void clearRows();

    QString m_userDir;
    QString m_language;

    EntryRow* m_keyRow = nullptr;
    SettingsGroup* m_summaryGroup = nullptr;
    SettingsGroup* m_listGroup = nullptr;
    QList<QWidget*> m_summaryRows;
    QList<QWidget*> m_trophyRows;
};
