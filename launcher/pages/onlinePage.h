#pragma once

#include <QList>
#include <QString>

#include "launcherPage.h"

class ChoiceRow;
class EntryRow;
class GroupProcess;
class QPushButton;
class SettingRow;
class SwitchRow;

// OnlinePage - online play through the online module (gpu/bbnet20): the pool (Seamless Hunt with
// BBSC seamless co-op, or The Hunter's Dream), its servers, the account, the Discord sign-in for
// The Hunter's Dream, and a connection test.
class OnlinePage : public LauncherPage
{
    Q_OBJECT

public:
    explicit OnlinePage(const LauncherSettings& settings, const PortIni& ini, QWidget* parent = nullptr);

    void store(LauncherSettings& settings, PortIni& ini) const override;

    // The build has the online module (gpu/libbbnet20.so, or the one BB_NET_MODULE names).
    [[nodiscard]] static bool hasOnlineModule();

    [[nodiscard]] bool playOnline() const;
    void setPlayOnline(bool on);
    [[nodiscard]] QString onlineId() const;

signals:
    // The Hunter's Dream sign-in finished: the window saves the token at once.
    void signedIn();

private:
    void showPool();
    void showThdAccount();
    void signIn();
    void testConnection();
    void showResults(const QJsonObject& answer);

    QString m_thdOnlineId;
    QString m_thdToken;
    QString m_signInError;
    bool m_signInCanceled = false;
    GroupProcess* m_signIn = nullptr;
    QList<QWidget*> m_resultRows;

    SwitchRow* m_onlineRow = nullptr;
    ChoiceRow* m_poolRow = nullptr;
    SettingsGroup* m_gameGroup = nullptr;
    EntryRow* m_communityRow = nullptr;
    SettingsGroup* m_coopGroup = nullptr;
    EntryRow* m_serverRow = nullptr;
    EntryRow* m_webapiRow = nullptr;
    SwitchRow* m_bbscRow = nullptr;
    SettingsGroup* m_accountGroup = nullptr;
    EntryRow* m_npidRow = nullptr;
    EntryRow* m_passwordRow = nullptr;
    SettingsGroup* m_thdGroup = nullptr;
    EntryRow* m_thdServerRow = nullptr;
    SettingRow* m_thdAccountRow = nullptr;
    QPushButton* m_thdButton = nullptr;
    QPushButton* m_thdLink = nullptr;
    SettingsGroup* m_connectionGroup = nullptr;
    SwitchRow* m_upnpRow = nullptr;
    QPushButton* m_testButton = nullptr;
};
