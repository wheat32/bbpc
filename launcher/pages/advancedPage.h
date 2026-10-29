#pragma once

#include "launcherPage.h"

class ChoiceRow;
class EntryRow;
class SwitchRow;

// AdvancedPage - the GPU pipeline, developer switches (logs, diagnostics, profiling, extra
// variables) and the launcher's own appearance.
class AdvancedPage : public LauncherPage
{
    Q_OBJECT

public:
    explicit AdvancedPage(const LauncherSettings& settings, const PortIni& ini, QWidget* parent = nullptr);

    void store(LauncherSettings& settings, PortIni& ini) const override;

signals:
    void themeChanged(const QString& theme);

private:
    ChoiceRow* m_pipeRow = nullptr;
    SwitchRow* m_mangohudRow = nullptr;
    SwitchRow* m_saveLogRow = nullptr;
    SwitchRow* m_crashDiagRow = nullptr;
    SwitchRow* m_statsRow = nullptr;
    SwitchRow* m_profileRow = nullptr;
    SwitchRow* m_validationRow = nullptr;
    EntryRow* m_extraRow = nullptr;
    ChoiceRow* m_themeRow = nullptr;
    SwitchRow* m_updatesRow = nullptr;
};
