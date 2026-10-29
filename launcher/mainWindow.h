#pragma once

#include <QMainWindow>

#include "core/portIni.h"

class AdvancedPage;
class ControlsPage;
class DisplayPage;
class GamePage;
class GraphicsPage;
class GroupProcess;
class LauncherPage;
class LogPage;
class ModsPage;
class OnlinePage;
class TrophiesPage;
class QLabel;
class QListWidget;
class QPushButton;
class QStackedWidget;
class QTimer;

// MainWindow - the pages in a sidebar, the game's status and the Play button. Starting the game
// runs run.sh with the environment the settings give (GameEnvironment); its output goes to the Log
// page. Settings are saved shortly after every change, before the game starts and on close.
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow();

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    enum class Page
    {
        Game,
        Display,
        Graphics,
        Controls,
        Mods,
        Trophies,
        Online,
        Advanced,
        Log,
    };

    void addPage(Page page, const QString& title, QWidget* widget);
    void showPage(Page page);
    void store();
    void scheduleSave();
    void onPlay();
    void startGame();
    void gameFinished(int exitCode);
    void updateStatus();
    void showToast(const QString& message);

    PortIni m_ini;
    bool m_hasGame = false;
    bool m_building = false;
    GroupProcess* m_game = nullptr;
    QTimer* m_saveTimer = nullptr;
    QList<LauncherPage*> m_settingsPages;

    QListWidget* m_sidebar = nullptr;
    QStackedWidget* m_stack = nullptr;
    QLabel* m_status = nullptr;
    QPushButton* m_playButton = nullptr;
    GamePage* m_gamePage = nullptr;
    DisplayPage* m_displayPage = nullptr;
    GraphicsPage* m_graphicsPage = nullptr;
    ControlsPage* m_controlsPage = nullptr;
    ModsPage* m_modsPage = nullptr;
    TrophiesPage* m_trophiesPage = nullptr;
    OnlinePage* m_onlinePage = nullptr;
    AdvancedPage* m_advancedPage = nullptr;
    LogPage* m_logPage = nullptr;
};
