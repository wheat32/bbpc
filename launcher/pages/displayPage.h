#pragma once

#include <QHash>
#include <QSize>
#include <QString>
#include <optional>

#include "launcherPage.h"

class ChoiceRow;
class SwitchRow;

// DisplayPage - the output's aspect ratio and resolution, live resolution changes, fullscreen,
// the monitor, the present mode, HDR and the FPS counter.
class DisplayPage : public LauncherPage
{
    Q_OBJECT

public:
    explicit DisplayPage(const LauncherSettings& settings, const PortIni& ini, QWidget* parent = nullptr);

    void store(LauncherSettings& settings, PortIni& ini) const override;

    // bbport.ini's output_res for the selected output ("WxH").
    [[nodiscard]] QString outputResolution() const;

signals:
    void outputResolutionChanged(const QString& output);

private:
    void fillDisplays();
    void fillAspects(const QString& current);
    void fillOutputs(const QSize& keep);
    [[nodiscard]] std::optional<QSize> monitorSize() const;

    QString m_savedDisplay;
    // The monitors' sizes by their BB_DISPLAY value; "" is the primary one.
    QHash<QString, QSize> m_displaySizes;

    ChoiceRow* m_aspectRow = nullptr;
    ChoiceRow* m_outputRow = nullptr;
    ChoiceRow* m_liveRow = nullptr;
    SwitchRow* m_fullscreenRow = nullptr;
    ChoiceRow* m_displayRow = nullptr;
    ChoiceRow* m_presentRow = nullptr;
    SwitchRow* m_hdrRow = nullptr;
    SwitchRow* m_showFpsRow = nullptr;
};
