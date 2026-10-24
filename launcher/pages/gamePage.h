#pragma once

#include <QList>
#include <QString>

#include "core/capabilities.h"
#include "launcherPage.h"

class ChoiceRow;
class QLabel;
class QToolButton;
class SettingRow;
class SpinRow;

// GamePage - the game folder (checked for the supported 1.09 executable), the saves folder, the
// game's text language, the memory model, the GPU and the frame rate.
class GamePage : public LauncherPage
{
    Q_OBJECT

public:
    explicit GamePage(const LauncherSettings& settings, const PortIni& ini, QWidget* parent = nullptr);

    void store(LauncherSettings& settings, PortIni& ini) const override;

    // The folder with eboot.bin, and whether it has one.
    [[nodiscard]] QString gameDir() const;
    [[nodiscard]] bool hasGame() const;
    // The saves folder: the chosen one or the data folder's user/.
    [[nodiscard]] QString userDir() const;
    // The GPUs bb-gpu-capabilities found (the game gets the chosen one by its index).
    [[nodiscard]] const QList<Capabilities::Gpu>& gpus() const
    {
        return m_gpus;
    }

public slots:
    // Checks the game folder again (after the game ran: a dump may have been replaced).
    void refreshGameStatus();

signals:
    void gameStatusChanged(bool hasGame);
    void userDirChanged(const QString& userDir);

private:
    void chooseGameDir();
    void chooseUserDir();
    void showGameStatus(const QString& problem);
    void showUserStatus();
    void fillGpus();

    QString m_gameDir;
    QString m_customUserDir;
    QString m_savedGpu;
    QList<Capabilities::Gpu> m_gpus;
    QString m_checkedKey;
    QString m_checkedProblem;

    SettingRow* m_gameRow = nullptr;
    QLabel* m_gameStatus = nullptr;
    SettingRow* m_userRow = nullptr;
    QLabel* m_userStatus = nullptr;
    QToolButton* m_userReset = nullptr;
    ChoiceRow* m_languageRow = nullptr;
    ChoiceRow* m_memoryModelRow = nullptr;
    ChoiceRow* m_gpuRow = nullptr;
    ChoiceRow* m_fpsRow = nullptr;
    SpinRow* m_fpsLimitRow = nullptr;
};
