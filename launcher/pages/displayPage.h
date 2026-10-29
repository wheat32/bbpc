#pragma once

#include <QHash>
#include <QSize>
#include <QString>
#include <optional>

#include "launcherPage.h"

class ChoiceRow;
class SwitchRow;

// DisplayPage - the output's aspect ratio and resolution (Auto: the monitor's own), fullscreen,
// the monitor, the present mode, HDR and the FPS counter. Resolution changes apply live in the
// game (always on since 0.7).
class DisplayPage : public LauncherPage
{
    Q_OBJECT

public:
    explicit DisplayPage(const LauncherSettings& settings, const PortIni& ini, QWidget* parent = nullptr);

    void store(LauncherSettings& settings, PortIni& ini) const override;

    // The selected output as "WxH" (Auto: the monitor's resolution).
    [[nodiscard]] QString outputResolution() const;

signals:
    // The selected output as "WxH" (Auto: the monitor's resolution).
    void outputResolutionChanged(const QString& output);

private:
    void fillDisplays();
    void fillAspects(const QString& current);
    // keep: the size to stay at (or the nearest of the aspect ratio); automatic: Auto selected.
    void fillOutputs(const QSize& keep, bool automatic);
    [[nodiscard]] bool outputIsAuto() const;
    [[nodiscard]] std::optional<QSize> monitorSize() const;

    QString m_savedDisplay;
    // The monitors' sizes by their BB_DISPLAY value; "" is the primary one.
    QHash<QString, QSize> m_displaySizes;

    ChoiceRow* m_aspectRow = nullptr;
    ChoiceRow* m_outputRow = nullptr;
    SwitchRow* m_fullscreenRow = nullptr;
    ChoiceRow* m_displayRow = nullptr;
    ChoiceRow* m_presentRow = nullptr;
    SwitchRow* m_hdrRow = nullptr;
    SwitchRow* m_showFpsRow = nullptr;
};
