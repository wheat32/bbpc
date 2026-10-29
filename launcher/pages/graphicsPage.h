#pragma once

#include <QHash>
#include <QList>
#include <QString>

#include "core/capabilities.h"
#include "launcherPage.h"

class ChoiceRow;
class GroupProcess;
class QPushButton;
class QToolButton;
class SettingRow;
class SpinRow;
class SwitchRow;

// GraphicsPage - the upscaler, frame generation and their options, anti-aliasing and image
// quality, the user's own AMD and NVIDIA files (FSR 4.1.1 built from AMD's DLL, FSR 4.1.2's models
// from AMD's driver, NVIDIA's DLSS libraries), and the game's effects (patches applied at start).
class GraphicsPage : public LauncherPage
{
    Q_OBJECT

public:
    explicit GraphicsPage(const LauncherSettings& settings, const PortIni& ini, QWidget* parent = nullptr);

    void store(LauncherSettings& settings, PortIni& ini) const override;

    [[nodiscard]] bool isBuilding() const;
    void cancelBuild();
    // The frame generation choice (Choices::frameGenerations).
    [[nodiscard]] QString frameGeneration() const;

public slots:
    void setOutputResolution(const QString& output);
    void setUserDir(const QString& userDir);
    // The GPUs found: DLSS Frame Generation is offered where one is NVIDIA's.
    void setGpus(const QList<Capabilities::Gpu>& gpus);
    // The game and an FSR 4.1 build do not run together: both want the GPU, and the build
    // replaces the assets the game reads.
    void setGameRunning(bool running);

signals:
    void buildRunningChanged(bool running);
    void logLine(const QString& line);
    void frameGenerationChanged(const QString& choice);

private:
    // What a build records: FSR 4.1.1 from the upscaler DLL, or FSR 4.1.2's models from the driver.
    enum class BuildKind
    {
        Fsr411,
        DriverModels,
    };

    void updateUpscalerStatus();
    void updateFrameGenerations();
    void updateFrameGenerationHint();
    void updateFsr411Row(const QString& progress = {});
    void updateDlssRow();
    void chooseFsr411Dll();
    void chooseDriver();
    void inspectDll(const QString& upscaler, const QString& loader);
    void startBuild(BuildKind kind, const QString& first, const QString& second);
    void buildFinished(int exitCode);
    void chooseDlssLibrary();
    void installDlssLibrary(const QString& path);
    void removeDlssLibrary();
    QString chooseDll(const QString& title);

    QString m_outputResolution;
    QString m_userDir;
    QString m_dllDir;
    QString m_fsr411Dll;
    QString m_frameGenerationMultiplier;
    QList<Capabilities::Gpu> m_gpus;
    bool m_gameRunning = false;
    bool m_buildCanceled = false;
    BuildKind m_buildKind = BuildKind::Fsr411;
    GroupProcess* m_build = nullptr;

    ChoiceRow* m_upscalerRow = nullptr;
    ChoiceRow* m_frameGenerationRow = nullptr;
    ChoiceRow* m_presetRow = nullptr;
    SwitchRow* m_sharpenRow = nullptr;
    SpinRow* m_sharpnessRow = nullptr;
    SwitchRow* m_motionRow = nullptr;
    ChoiceRow* m_antiAliasingRow = nullptr;
    ChoiceRow* m_textureRow = nullptr;
    ChoiceRow* m_lodRow = nullptr;
    SwitchRow* m_vbaoRow = nullptr;
    SettingRow* m_fsr411Row = nullptr;
    QPushButton* m_fsr411Button = nullptr;
    SettingRow* m_fsr412Row = nullptr;
    QPushButton* m_fsr412Button = nullptr;
    SettingRow* m_dlssRow = nullptr;
    QToolButton* m_dlssReset = nullptr;
    QHash<QString, SwitchRow*> m_effectRows;
    SwitchRow* m_skipNetworkRow = nullptr;
};
