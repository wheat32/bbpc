#pragma once

#include "launcherPage.h"

class ChoiceRow;
class EntryRow;
class SwitchRow;

// AdvancedPage - performance options, developer switches (diagnostics, profiling, extra
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
    ChoiceRow* m_readbacksRow = nullptr;
    ChoiceRow* m_preuploadRow = nullptr;
    SwitchRow* m_mangohudRow = nullptr;
    SwitchRow* m_saveLogRow = nullptr;
    SwitchRow* m_crashDiagRow = nullptr;
    SwitchRow* m_as03Row = nullptr;
    SwitchRow* m_statsRow = nullptr;
    SwitchRow* m_profileRow = nullptr;
    SwitchRow* m_validationRow = nullptr;
    EntryRow* m_extraRow = nullptr;
    ChoiceRow* m_themeRow = nullptr;
};
