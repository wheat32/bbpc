#include "mainWindow.h"

#include <QCloseEvent>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPushButton>
#include <QStackedWidget>
#include <QStyle>
#include <QTimer>
#include <QVBoxLayout>

#include "core/appConfig.h"
#include "core/gameEnvironment.h"
#include "core/hostEnvironment.h"
#include "core/portPaths.h"
#include "core/updateCheck.h"
#include "debug.h"
#include "dialogs/updateAvailableDialog.h"
#include "pages/advancedPage.h"
#include "pages/controlsPage.h"
#include "pages/displayPage.h"
#include "pages/gamePage.h"
#include "pages/graphicsPage.h"
#include "pages/logPage.h"
#include "pages/modsPage.h"
#include "pages/onlinePage.h"
#include "pages/trophiesPage.h"
#include "process/groupProcess.h"
#include "themeManager.h"
#include "widgets/toastNotification.h"

namespace
{
constexpr int SAVE_DELAY_MS = 600;
constexpr int SIDEBAR_WIDTH = 190;
constexpr int SIDEBAR_MARGIN = 12;
constexpr int BAR_MARGIN_H = 18;
constexpr int BAR_MARGIN_V = 12;
constexpr int PLAY_BUTTON_MIN_WIDTH = 150;
constexpr int CONTENT_STRETCH = 1;
constexpr int UPDATE_CHECK_DELAY_MS = 3000;
constexpr int UPDATE_CHECK_TIMEOUT_MS = 10000;
} // namespace

MainWindow::MainWindow()
{
    setWindowTitle(QStringLiteral("bbpc"));
    m_ini.load(PortPaths::iniPath());
    const LauncherSettings& settings = AppConfig::instance().settings();

    m_game = new GroupProcess(this);
    m_saveTimer = new QTimer(this);
    m_saveTimer->setSingleShot(true);
    m_saveTimer->setInterval(SAVE_DELAY_MS);
    connect(m_saveTimer, &QTimer::timeout, this, &MainWindow::store);

    QWidget* central = new QWidget(this);
    QHBoxLayout* rootLayout = new QHBoxLayout(central);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    QWidget* side = new QWidget(central);
    side->setObjectName(QStringLiteral("sidebar"));
    side->setFixedWidth(SIDEBAR_WIDTH);
    QVBoxLayout* sideLayout = new QVBoxLayout(side);
    sideLayout->setContentsMargins(SIDEBAR_MARGIN, SIDEBAR_MARGIN, SIDEBAR_MARGIN, SIDEBAR_MARGIN);
    QLabel* name = new QLabel(QStringLiteral("bbpc"), side);
    name->setObjectName(QStringLiteral("appName"));
    QLabel* tagline = new QLabel(tr("Bloodborne on Linux"), side);
    tagline->setObjectName(QStringLiteral("appTagline"));
    sideLayout->addWidget(name);
    sideLayout->addWidget(tagline);
    m_sidebar = new QListWidget(side);
    m_sidebar->setObjectName(QStringLiteral("sidebarList"));
    m_sidebar->setFrameShape(QFrame::NoFrame);
    sideLayout->addWidget(m_sidebar, CONTENT_STRETCH);
    rootLayout->addWidget(side);

    QWidget* content = new QWidget(central);
    QVBoxLayout* contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(0);
    m_stack = new QStackedWidget(content);
    contentLayout->addWidget(m_stack, CONTENT_STRETCH);

    QWidget* bar = new QWidget(content);
    bar->setObjectName(QStringLiteral("actionBar"));
    QHBoxLayout* barLayout = new QHBoxLayout(bar);
    barLayout->setContentsMargins(BAR_MARGIN_H, BAR_MARGIN_V, BAR_MARGIN_H, BAR_MARGIN_V);
    m_status = new QLabel(bar);
    m_status->setObjectName(QStringLiteral("statusLabel"));
    m_status->setWordWrap(true);
    m_playButton = new QPushButton(bar);
    m_playButton->setObjectName(QStringLiteral("playButton"));
    m_playButton->setMinimumWidth(PLAY_BUTTON_MIN_WIDTH);
    m_playButton->setCursor(Qt::PointingHandCursor);
    barLayout->addWidget(m_status, CONTENT_STRETCH);
    barLayout->addWidget(m_playButton);
    contentLayout->addWidget(bar);
    rootLayout->addWidget(content, CONTENT_STRETCH);
    setCentralWidget(central);

    m_gamePage = new GamePage(settings, m_ini);
    m_displayPage = new DisplayPage(settings, m_ini);
    m_graphicsPage = new GraphicsPage(settings, m_ini);
    m_controlsPage = new ControlsPage(settings, m_ini);
    m_modsPage = new ModsPage(settings, m_ini);
    m_trophiesPage = new TrophiesPage(settings, m_ini);
    m_onlinePage = new OnlinePage(settings, m_ini);
    m_advancedPage = new AdvancedPage(settings, m_ini);
    m_logPage = new LogPage();
    addPage(Page::Game, tr("Game"), m_gamePage);
    addPage(Page::Display, tr("Display"), m_displayPage);
    addPage(Page::Graphics, tr("Graphics"), m_graphicsPage);
    addPage(Page::Controls, tr("Controls"), m_controlsPage);
    addPage(Page::Mods, tr("Mods and patches"), m_modsPage);
    addPage(Page::Trophies, tr("Trophies"), m_trophiesPage);
    addPage(Page::Online, tr("Online"), m_onlinePage);
    addPage(Page::Advanced, tr("Advanced"), m_advancedPage);
    addPage(Page::Log, tr("Log"), m_logPage);
    m_settingsPages = { m_gamePage, m_displayPage, m_graphicsPage, m_controlsPage, m_modsPage, m_trophiesPage, m_onlinePage,
                        m_advancedPage };
    connect(m_sidebar, &QListWidget::currentRowChanged, m_stack, &QStackedWidget::setCurrentIndex);
    connect(m_sidebar, &QListWidget::currentRowChanged, this, [this](int row)
    {
        // The game may have unlocked some since the page was last shown.
        if (row == static_cast<int>(Page::Trophies))
        {
            m_trophiesPage->refresh();
        }
    });

    for (LauncherPage* page : m_settingsPages)
    {
        connect(page, &LauncherPage::changed, this, &MainWindow::scheduleSave);
        connect(page, &LauncherPage::toast, this, &MainWindow::showToast);
    }
    connect(m_logPage, &LogPage::toast, this, &MainWindow::showToast);
    connect(m_gamePage, &GamePage::gameStatusChanged, this, [this](bool hasGame)
    {
        m_hasGame = hasGame;
        updateStatus();
    });
    connect(m_gamePage, &GamePage::userDirChanged, m_graphicsPage, &GraphicsPage::setUserDir);
    connect(m_gamePage, &GamePage::userDirChanged, m_trophiesPage, &TrophiesPage::setUserDir);
    connect(m_gamePage, &GamePage::systemLanguageChanged, m_trophiesPage, &TrophiesPage::setLanguage);
    connect(m_gamePage, &GamePage::gpusChanged, m_graphicsPage, &GraphicsPage::setGpus);
    connect(m_graphicsPage, &GraphicsPage::frameGenerationChanged, m_gamePage, &GamePage::setFrameGeneration);
    connect(m_displayPage, &DisplayPage::outputResolutionChanged, m_graphicsPage, &GraphicsPage::setOutputResolution);
    connect(m_graphicsPage, &GraphicsPage::logLine, m_logPage, &LogPage::appendLine);
    connect(m_graphicsPage, &GraphicsPage::buildRunningChanged, this, [this](bool running)
    {
        m_building = running;
        if (running)
        {
            showPage(Page::Log);
        }
        updateStatus();
    });
    connect(m_onlinePage, &OnlinePage::signedIn, this, &MainWindow::store);
    connect(m_advancedPage, &AdvancedPage::themeChanged, this, [](const QString& theme)
    {
        ThemeManager::apply(theme);
    });
    connect(m_game, &GroupProcess::lineReceived, m_logPage, &LogPage::appendLine);
    connect(m_game, &GroupProcess::finished, this, &MainWindow::gameFinished);
    connect(m_playButton, &QPushButton::clicked, this, &MainWindow::onPlay);

    m_hasGame = m_gamePage->hasGame();
    m_graphicsPage->setUserDir(m_gamePage->userDir());
    m_graphicsPage->setOutputResolution(m_displayPage->outputResolution());
    m_gamePage->setFrameGeneration(m_graphicsPage->frameGeneration());
    m_trophiesPage->setLanguage(m_gamePage->systemLanguage());
    m_trophiesPage->setUserDir(m_gamePage->userDir());
    showPage(Page::Game);
    updateStatus();
    QTimer::singleShot(UPDATE_CHECK_DELAY_MS, this, &MainWindow::checkForUpdates);
}

void MainWindow::checkForUpdates()
{
    if (AppConfig::instance().settings().checkForUpdates == false) return;
    if (m_network == nullptr)
    {
        m_network = new QNetworkAccessManager(this);
    }
    DBG_APP(QStringLiteral("Checking for updates..."));
    QNetworkRequest request{ QUrl(UpdateCheck::VERSION_URL) };
    request.setTransferTimeout(UPDATE_CHECK_TIMEOUT_MS);
    QNetworkReply* reply = m_network->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]()
    {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError)
        {
            DBG_APP(QStringLiteral("Update check failed: ") + reply->errorString());
            return;
        }
        const QString remoteVersion = UpdateCheck::remoteVersion(reply->readAll());
        const QString localVersion = QCoreApplication::applicationVersion();
        if (UpdateCheck::isNewer(remoteVersion, localVersion) == false)
        {
            DBG_APP(QStringLiteral("Up to date (v%1; GitHub: v%2)").arg(localVersion, remoteVersion));
            return;
        }
        DBG_APP(QStringLiteral("Update available: v%1 -> v%2").arg(localVersion, remoteVersion));
        UpdateAvailableDialog* dialog = new UpdateAvailableDialog(localVersion, remoteVersion, this);
        dialog->setModal(true);
        dialog->show();
    });
}

void MainWindow::addPage(Page page, const QString& title, QWidget* widget)
{
    m_stack->insertWidget(static_cast<int>(page), widget);
    m_sidebar->insertItem(static_cast<int>(page), title);
}

void MainWindow::showPage(Page page)
{
    m_sidebar->setCurrentRow(static_cast<int>(page));
}

void MainWindow::scheduleSave()
{
    m_saveTimer->start();
}

void MainWindow::store()
{
    m_saveTimer->stop();
    LauncherSettings& settings = AppConfig::instance().settings();
    PortIni edited = m_ini;
    for (LauncherPage* page : m_settingsPages)
    {
        page->store(settings, edited);
    }
    AppConfig::instance().save();
    // Only what was changed here: the in-game menu edits the same file while the game runs.
    if (PortIni::saveChanges(PortPaths::iniPath(), m_ini, edited) == false)
    {
        DBG_SETTINGS(QStringLiteral("Could not write ") + PortPaths::iniPath());
    }
    m_ini = edited;
}

void MainWindow::updateStatus()
{
    const bool running = m_game->isRunning();
    m_playButton->setText(running ? tr("Stop") : tr("Play"));
    m_playButton->setProperty("running", running);
    m_playButton->style()->unpolish(m_playButton);
    m_playButton->style()->polish(m_playButton);
    m_playButton->setEnabled(running || (m_hasGame && m_building == false));
    m_graphicsPage->setGameRunning(running);
    if (running)
    {
        m_status->setText(tr("The game is running"));
    }
    else if (m_building)
    {
        m_status->setText(tr("Building FSR 4.1 (the game can start once it is done)"));
    }
    else if (PortPaths::portDir().isEmpty())
    {
        m_status->setText(tr("The port's files (run.sh, scripts/) were not found next to the launcher"));
    }
    else if (m_hasGame == false)
    {
        m_status->setText(tr("Choose the game folder (CUSA03173 with update 1.09) on the Game page"));
    }
    else
    {
        m_status->setText(tr("Ready"));
    }
}

void MainWindow::onPlay()
{
    if (m_game->isRunning())
    {
        m_game->stop();
        return;
    }
    // An account is filled in, but online play is off: ask which one was meant.
    if (OnlinePage::hasOnlineModule() && m_onlinePage->playOnline() == false && m_onlinePage->onlineId().isEmpty() == false)
    {
        QMessageBox box(QMessageBox::Question, QStringLiteral("bbpc"),
                        tr("An online account is filled in, but \"Play online\" is off"), QMessageBox::NoButton, this);
        QPushButton* offline = box.addButton(tr("Play offline"), QMessageBox::RejectRole);
        QPushButton* online = box.addButton(tr("Play online"), QMessageBox::AcceptRole);
        box.addButton(QMessageBox::Cancel);
        box.setDefaultButton(online);
        box.exec();
        if (box.clickedButton() != offline && box.clickedButton() != online) return;
        m_onlinePage->setPlayOnline(box.clickedButton() == online);
    }
    startGame();
}

void MainWindow::startGame()
{
    if (m_onlinePage->playOnline() && m_onlinePage->onlineId().contains(u'@'))
    {
        QMessageBox::information(this, QStringLiteral("bbpc"),
                                 tr("The Online ID is the account name (NPID) you registered on the server, not your "
                                    "email address"));
        showPage(Page::Online);
        return;
    }
    store();
    m_logPage->clear();
    GameEnvironment::Location location;
    location.dataDir = PortPaths::dataDir();
    location.packaged = PortPaths::packaged();
    location.buildDir = PortPaths::buildDir();
    location.onlineModule = OnlinePage::hasOnlineModule();
    const QProcessEnvironment env = GameEnvironment::build(AppConfig::instance().settings(), HostEnvironment::host(),
                                                           location, m_gamePage->gpus());
    QDir().mkpath(location.dataDir);
    if (m_game->start(QStringLiteral("bash"), { PortPaths::portDir() + QStringLiteral("/run.sh") }, env,
                      PortPaths::portDir()) == false)
    {
        showToast(tr("Could not start: %1").arg(m_game->errorString()));
        return;
    }
    showPage(Page::Log);
    updateStatus();
}

void MainWindow::gameFinished(int exitCode)
{
    m_logPage->appendLine(QString());
    m_logPage->appendLine(tr("--- the game exited (exit code %1) ---").arg(exitCode));
    DBG_GAME(QStringLiteral("The game exited with code ") + QString::number(exitCode));
    updateStatus();
    m_gamePage->refreshGameStatus();
    // What this session unlocked, and the names once the game decrypted them.
    m_trophiesPage->refresh();
}

void MainWindow::showToast(const QString& message)
{
    ToastNotification::popup(this, message);
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    store();
    // The game and a build in progress stop with the launcher (GroupProcess waits for them).
    m_graphicsPage->cancelBuild();
    m_game->stop();
    QMainWindow::closeEvent(event);
}
