#pragma once

#include <QHash>
#include <QString>

#include "launcherPage.h"

class ChoiceRow;
class GroupProcess;
class QPushButton;
class QToolButton;
class SettingRow;
class SpinRow;
class SwitchRow;

// GraphicsPage - the upscaler and its options, FSR 4.1.1 built from the user's own AMD DLL, NVIDIA's
// DLSS library, and the game's effects (patches applied at start).
class GraphicsPage : public LauncherPage
{
    Q_OBJECT

public:
    explicit GraphicsPage(const LauncherSettings& settings, const PortIni& ini, QWidget* parent = nullptr);

    void store(LauncherSettings& settings, PortIni& ini) const override;

    [[nodiscard]] bool isBuilding() const;
    void cancelBuild();

public slots:
    void setOutputResolution(const QString& output);
    void setUserDir(const QString& userDir);
    // The game and an FSR 4.1.1 build do not run together: both want the GPU, and the build
    // replaces the assets the game reads.
    void setGameRunning(bool running);

signals:
    void buildRunningChanged(bool running);
    void logLine(const QString& line);

private:
    void updateUpscalerStatus();
    void updateFsr411Row(const QString& progress = {});
    void updateDlssRow();
    void chooseFsr411Dll();
    void inspectDll(const QString& upscaler, const QString& loader);
    void startBuild(const QString& upscaler, const QString& loader);
    void buildFinished(int exitCode);
    void chooseDlssLibrary();
    void installDlssLibrary(const QString& path);
    void removeDlssLibrary();
    QString chooseDll(const QString& title);

    QString m_outputResolution;
    QString m_userDir;
    QString m_dllDir;
    bool m_gameRunning = false;
    bool m_buildCanceled = false;
    GroupProcess* m_build = nullptr;

    ChoiceRow* m_upscalerRow = nullptr;
    SettingRow* m_fsr411Row = nullptr;
    QPushButton* m_fsr411Button = nullptr;
    SettingRow* m_dlssRow = nullptr;
    QToolButton* m_dlssReset = nullptr;
    ChoiceRow* m_presetRow = nullptr;
    SwitchRow* m_sharpenRow = nullptr;
    SpinRow* m_sharpnessRow = nullptr;
    SwitchRow* m_motionRow = nullptr;
    ChoiceRow* m_lodRow = nullptr;
    QHash<QString, SwitchRow*> m_effectRows;
    SwitchRow* m_skipNetworkRow = nullptr;
};
