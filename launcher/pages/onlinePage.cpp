#include "onlinePage.h"

#include <QClipboard>
#include <QDesktopServices>
#include <QFileInfo>
#include <QGuiApplication>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QPushButton>
#include <QUrl>

#include "core/choices.h"
#include "core/hostEnvironment.h"
#include "core/onlineAddresses.h"
#include "core/portPaths.h"
#include "process/groupProcess.h"
#include "process/portQueries.h"
#include "widgets/settingRows.h"
#include "widgets/uiHelpers.h"

namespace
{
constexpr int CHECK_TIMEOUT_MS = 30000;
const QString HUNTERS_DREAM = QStringLiteral("huntersdream");
} // namespace

bool OnlinePage::hasOnlineModule()
{
    return QFileInfo(PortPaths::binariesDir() + QStringLiteral("/gpu/libbbnet20.so")).isFile();
}

OnlinePage::OnlinePage(const LauncherSettings& settings, const PortIni&, QWidget* parent)
    : LauncherPage(parent), m_thdOnlineId(settings.thdOnlineId), m_thdToken(settings.thdToken)
{
    m_signIn = new GroupProcess(this);

    SettingsGroup* play = new SettingsGroup({}, tr("Messages, bloodstains and ghosts come from the game server; bells and "
                                                   "summons go through a shadNet server. By default both are Seamless "
                                                   "Hunt's (BBSC's server). Online play works together with the Windows "
                                                   "version: everyone needs game version 1.09 and the same server"));
    m_onlineRow = new SwitchRow(tr("Play online"), tr("Off: the game stays offline, as before"), settings.online);
    connect(m_onlineRow, &SwitchRow::toggled, this, &OnlinePage::changed);
    if (hasOnlineModule() == false)
    {
        m_onlineRow->setEnabled(false);
        m_onlineRow->setSubtitle(tr("This build has no online module (libbbnet20.so): offline play only"));
    }
    play->addRow(m_onlineRow);
    m_poolRow = new ChoiceRow(tr("Pool"), tr("Who to play with: everyone in a session must use the same pool"),
                              Choices::onlinePools(), settings.pool());
    connect(m_poolRow, &ChoiceRow::valueChanged, this, [this]()
    {
        showPool();
        emit changed();
    });
    play->addRow(m_poolRow);
    addGroup(play);

    m_gameGroup = new SettingsGroup(tr("Game server (messages, bloodstains, ghosts)"));
    m_communityRow = new EntryRow(tr("Game server"), settings.onlineCommunity);
    connect(m_communityRow, &EntryRow::textEdited, this, &OnlinePage::changed);
    m_gameGroup->addRow(m_communityRow);
    addGroup(m_gameGroup);

    m_coopGroup = new SettingsGroup(tr("Co-op server (bells and summons)"),
                                    tr("play.seamlesshunt.com:31313 is Seamless Hunt's server (BBSC). shadPS4's public "
                                       "server accepts only shadPS4 builds. For a private server, enter host:port from the "
                                       "person who runs it. WebAPI: leave it empty to use the server address with port "
                                       "31315"));
    m_serverRow = new EntryRow(tr("Server address"), settings.onlineServer);
    m_webapiRow = new EntryRow(tr("WebAPI address"), settings.onlineWebapi);
    connect(m_serverRow, &EntryRow::textEdited, this, &OnlinePage::changed);
    connect(m_webapiRow, &EntryRow::textEdited, this, &OnlinePage::changed);
    m_coopGroup->addRow(m_serverRow);
    m_coopGroup->addRow(m_webapiRow);
    m_bbscRow = new SwitchRow(tr("Seamless co-op (BBSC)"),
                              tr("The session survives deaths, lamps, area changes and bosses; up to 5 hunters. Needs a "
                                 "server running the BBSC game server, and every player turns it on"),
                              settings.onlineBbsc);
    connect(m_bbscRow, &SwitchRow::toggled, this, &OnlinePage::changed);
    m_coopGroup->addRow(m_bbscRow);
    addGroup(m_coopGroup);

    m_accountGroup = new SettingsGroup(tr("Account"),
                                       tr("Your shadNet account name, not your email. Register at seamlesshunt.com or "
                                          "ask the owner of a private server. The password is saved on this computer "
                                          "with the launcher settings"));
    m_npidRow = new EntryRow(tr("Online ID (NPID)"), settings.onlineNpid);
    m_passwordRow = new EntryRow(tr("Password"), settings.onlinePassword, true);
    connect(m_npidRow, &EntryRow::textEdited, this, &OnlinePage::changed);
    connect(m_passwordRow, &EntryRow::textEdited, this, &OnlinePage::changed);
    m_accountGroup->addRow(m_npidRow);
    m_accountGroup->addRow(m_passwordRow);
    addGroup(m_accountGroup);

    m_thdGroup = new SettingsGroup(QStringLiteral("The Hunter's Dream"),
                                   tr("Discord sign-in as in bbhost: confirm a code on the website. No password; the "
                                      "token is kept on this computer with the launcher settings"));
    m_thdServerRow = new EntryRow(tr("Server address"), settings.thdServer);
    m_thdServerRow->setPlaceholder(OnlineAddresses::HUNTERS_DREAM_SERVER);
    connect(m_thdServerRow, &EntryRow::textEdited, this, &OnlinePage::changed);
    m_thdGroup->addRow(m_thdServerRow);
    m_thdAccountRow = new SettingRow(QString());
    m_thdButton = new QPushButton(m_thdAccountRow);
    m_thdAccountRow->addSuffix(m_thdButton);
    connect(m_thdButton, &QPushButton::clicked, this, &OnlinePage::signIn);
    m_thdGroup->addRow(m_thdAccountRow);
    addGroup(m_thdGroup);

    m_connectionGroup = new SettingsGroup(tr("Connection"),
                                          tr("Everyone in a session needs the same game version (1.09) and the same "
                                             "server. Cheats change the game for the other players too: turn them off "
                                             "when you play together"));
    m_upnpRow = new SwitchRow(tr("UPnP (open a port on the router automatically)"),
                              tr("Turn it off with Tailscale or another VPN"), settings.onlineUpnp);
    connect(m_upnpRow, &SwitchRow::toggled, this, &OnlinePage::changed);
    m_connectionGroup->addRow(m_upnpRow);
    SettingRow* testRow = new SettingRow(tr("Test the connection"));
    m_testButton = new QPushButton(tr("Test"), testRow);
    testRow->addSuffix(m_testButton);
    connect(m_testButton, &QPushButton::clicked, this, &OnlinePage::testConnection);
    m_connectionGroup->addRow(testRow);
    addGroup(m_connectionGroup);
    finish();

    connect(m_signIn, &GroupProcess::lineReceived, this, [this](const QString& line)
    {
        const QJsonObject event = QJsonDocument::fromJson(line.toUtf8()).object();
        const QString kind = event.value(QStringLiteral("event")).toString();
        if (kind == QStringLiteral("code"))
        {
            const QString page = event.value(QStringLiteral("page")).toString();
            const QString code = event.value(QStringLiteral("code")).toString();
            m_thdAccountRow->setTitle(tr("Sign-in code: %1").arg(code));
            m_thdAccountRow->setSubtitle(tr("On %1: 1) press \"Sign in with Discord to link\" and sign in; 2) back on "
                                            "the page, enter the code in the code field (not in \"Sign in with a code "
                                            "from the game\"). The code is on the clipboard").arg(page));
            QGuiApplication::clipboard()->setText(code);
            // The page opens in the browser; when nothing opens it, a button does.
            if (QDesktopServices::openUrl(QUrl(page)) == false && m_thdLink == nullptr)
            {
                m_thdLink = new QPushButton(tr("Open the page"), m_thdAccountRow);
                connect(m_thdLink, &QPushButton::clicked, this, [page]()
                {
                    QDesktopServices::openUrl(QUrl(page));
                });
                m_thdAccountRow->addSuffix(m_thdLink);
            }
        }
        else if (kind == QStringLiteral("signed-in"))
        {
            m_thdOnlineId = event.value(QStringLiteral("online_id")).toString();
            m_thdToken = event.value(QStringLiteral("token")).toString();
            emit signedIn();
        }
        else if (kind == QStringLiteral("error"))
        {
            m_signInError = event.value(QStringLiteral("message")).toString();
        }
    });
    connect(m_signIn, &GroupProcess::finished, this, [this]()
    {
        const QString error = m_signInCanceled ? QString() : m_signInError;
        m_signInCanceled = false;
        m_signInError.clear();
        showThdAccount();
        if (error.isEmpty() == false)
        {
            m_thdAccountRow->setSubtitle(error);
        }
    });

    showThdAccount();
    showPool();
}

void OnlinePage::store(LauncherSettings& settings, PortIni&) const
{
    settings.online = m_onlineRow->isOn();
    settings.onlinePool = m_poolRow->value();
    settings.onlineBbsc = m_bbscRow->isOn();
    settings.onlineCommunity = m_communityRow->text().trimmed();
    settings.onlineServer = m_serverRow->text().trimmed();
    settings.onlineWebapi = m_webapiRow->text().trimmed();
    settings.onlineNpid = m_npidRow->text().trimmed();
    settings.onlinePassword = m_passwordRow->text();
    settings.onlineUpnp = m_upnpRow->isOn();
    settings.thdServer = m_thdServerRow->text().trimmed();
    settings.thdOnlineId = m_thdOnlineId;
    settings.thdToken = m_thdToken;
}

bool OnlinePage::playOnline() const
{
    return m_onlineRow->isOn();
}

void OnlinePage::setPlayOnline(bool on)
{
    m_onlineRow->setOn(on);
}

QString OnlinePage::onlineId() const
{
    return m_npidRow->text().trimmed();
}

void OnlinePage::showPool()
{
    const bool thd = m_poolRow->value() == HUNTERS_DREAM;
    m_gameGroup->setVisible(thd == false);
    m_coopGroup->setVisible(thd == false);
    m_accountGroup->setVisible(thd == false);
    m_bbscRow->setVisible(thd == false);
    m_thdGroup->setVisible(thd);
    for (QWidget* row : m_resultRows)
    {
        m_connectionGroup->removeRow(row);
    }
    m_resultRows.clear();
    if (hasOnlineModule() == false)
    {
        m_poolRow->setBaseSubtitle(tr("This build has no module for this pool (libbbnet20.so)"));
    }
}

void OnlinePage::showThdAccount()
{
    if (m_thdLink != nullptr)
    {
        m_thdAccountRow->removeWidget(m_thdLink);
        m_thdLink = nullptr;
    }
    if (m_thdOnlineId.isEmpty() == false && m_thdToken.isEmpty() == false)
    {
        m_thdAccountRow->setTitle(tr("Signed in as %1").arg(m_thdOnlineId));
        m_thdAccountRow->setSubtitle(QString());
        m_thdButton->setText(tr("Sign out"));
    }
    else
    {
        m_thdAccountRow->setTitle(tr("Not signed in"));
        m_thdAccountRow->setSubtitle(tr("A Discord account linked to The Hunter's Dream is needed"));
        m_thdButton->setText(tr("Sign in with Discord"));
    }
    if (m_signIn->isRunning())
    {
        m_thdButton->setText(tr("Cancel"));
    }
}

void OnlinePage::signIn()
{
    if (m_signIn->isRunning())
    {
        // The sign-in stops; its end shows the account row again.
        m_signInCanceled = true;
        m_signIn->stop();
        return;
    }
    if (m_thdOnlineId.isEmpty() == false && m_thdToken.isEmpty() == false)
    {
        m_thdOnlineId.clear();
        m_thdToken.clear();
        showThdAccount();
        emit changed();
        return;
    }
    const QStringList arguments = PortQueries::scriptArguments({ QStringLiteral("thd-sign-in"), QStringLiteral("--server"),
                                                                 m_thdServerRow->text().trimmed() });
    m_signInError.clear();
    if (m_signIn->start(PortQueries::python(), arguments, HostEnvironment::host(), PortPaths::portDir()) == false)
    {
        emit toast(tr("Could not start: %1").arg(m_signIn->errorString()));
        return;
    }
    showThdAccount();
    m_thdAccountRow->setSubtitle(tr("Asking The Hunter's Dream for a code..."));
}

void OnlinePage::testConnection()
{
    for (QWidget* row : m_resultRows)
    {
        m_connectionGroup->removeRow(row);
    }
    m_resultRows.clear();
    m_testButton->setEnabled(false);
    m_testButton->setText(tr("Checking..."));
    QStringList arguments;
    if (m_poolRow->value() == HUNTERS_DREAM)
    {
        arguments = { QStringLiteral("thd-check"), QStringLiteral("--server"), m_thdServerRow->text().trimmed() };
    }
    else
    {
        const QString server = m_serverRow->text().trimmed().isEmpty() ? OnlineAddresses::SHADNET_SERVER
                                                                       : m_serverRow->text().trimmed();
        const QString community = OnlineAddresses::communityAddress(m_communityRow->text());
        const QString webapi = OnlineAddresses::webapiSetting(m_webapiRow->text(), server, community);
        arguments = { QStringLiteral("online-check"), QStringLiteral("--server"), server, QStringLiteral("--webapi"),
                      webapi, QStringLiteral("--community"), community };
    }
    PortQueries::query(arguments, this, [this](const QJsonObject& answer)
    {
        showResults(answer);
    }, CHECK_TIMEOUT_MS);
}

void OnlinePage::showResults(const QJsonObject& answer)
{
    m_testButton->setEnabled(true);
    m_testButton->setText(tr("Test"));
    if (answer.contains(PortQueries::ERROR_KEY))
    {
        SettingRow* row = new SettingRow(tr("The test did not run"), answer.value(PortQueries::ERROR_KEY).toString());
        m_connectionGroup->addRow(row);
        m_resultRows.append(row);
        return;
    }
    const QHash<QString, QString> names = {
        { QStringLiteral("game"), tr("Game server") },
        { QStringLiteral("shadnet"), tr("shadNet server") },
        { QStringLiteral("webapi"), QStringLiteral("WebAPI") },
        { QStringLiteral("service"), tr("Co-op service") },
    };
    for (const QJsonValue& value : answer.value(QStringLiteral("results")).toArray())
    {
        const QJsonObject result = value.toObject();
        const bool ok = result.value(QStringLiteral("ok")).toBool();
        const QString name = names.value(result.value(QStringLiteral("name")).toString(), result.value(QStringLiteral("name")).toString());
        const QString subtitle = ok ? tr("reachable")
                                    : tr("not reachable (%1)").arg(result.value(QStringLiteral("detail")).toString());
        SettingRow* row = new SettingRow(name + QStringLiteral(": ") + result.value(QStringLiteral("address")).toString(), subtitle);
        QLabel* icon = UiHelpers::statusIcon(row);
        UiHelpers::setStatusIcon(icon, ok ? Icons::Kind::Ok : Icons::Kind::Warning, subtitle);
        row->addPrefix(icon);
        m_connectionGroup->addRow(row);
        m_resultRows.append(row);
    }
}
