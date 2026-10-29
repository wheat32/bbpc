#include "advancedPage.h"

#include <QCoreApplication>
#include <QPushButton>
#include <QToolButton>

#include "core/appConfig.h"
#include "core/choices.h"
#include "core/portPaths.h"
#include "dialogs/aboutDialog.h"
#include "widgets/settingRows.h"
#include "widgets/uiHelpers.h"

AdvancedPage::AdvancedPage(const LauncherSettings& settings, const PortIni&, QWidget* parent) : LauncherPage(parent)
{
    // 0.7 has one memory model: the 0.3 synchronization, the readback and pre-upload choices are
    // gone (BB_READBACKS still works as an extra variable).
    SettingsGroup* performance = new SettingsGroup(tr("Performance"));
    m_pipeRow = new ChoiceRow(tr("Two-stage GPU pipeline"),
                              tr("Command decoding and draw recording on separate threads: 20-30% faster; turn it off "
                                 "if the game is unstable"),
                              Choices::drawPipe(), settings.drawPipe);
    connect(m_pipeRow, &ChoiceRow::valueChanged, this, &AdvancedPage::changed);
    performance->addRow(m_pipeRow);
    addGroup(performance);

    SettingsGroup* developer = new SettingsGroup(tr("Developer"), tr("Logs and diagnostics: for stutters and crashes"));
    m_mangohudRow = new SwitchRow(QStringLiteral("MangoHud"),
                                  tr("Its Vulkan layer must be installed (Arch: mangohud). Shows the game's RAM and VRAM "
                                     "too unless MANGOHUD_CONFIG is set"),
                                  settings.mangohud);
    m_saveLogRow = new SwitchRow(tr("Save the log and statistics to a file"),
                                 tr("Into the logs folder of the data folder: to look into stutters and crashes"),
                                 settings.saveLog);
    m_crashDiagRow = new SwitchRow(tr("Crash diagnostics"),
                                   tr("Checks the game's heap and the writes into its memory; a little slower"),
                                   settings.crashDiag);
    m_statsRow = new SwitchRow(tr("Frame statistics in the log"),
                               tr("Every few seconds: FPS, the worst frame, thread load, memory (BB_FRAME_STATS)"),
                               settings.frameStats);
    m_profileRow = new SwitchRow(tr("GPU profile in the log"),
                                 tr("The GPU time of each pass; slows the GPU down noticeably itself (BB_GPU_PROFILE)"),
                                 settings.gpuProfile);
    m_validationRow = new SwitchRow(tr("Vulkan validation layers"), tr("Checks of the Vulkan calls; much slower"),
                                    settings.vkValidation);
    for (SwitchRow* row : { m_mangohudRow, m_saveLogRow, m_crashDiagRow, m_statsRow, m_profileRow, m_validationRow })
    {
        connect(row, &SwitchRow::toggled, this, &AdvancedPage::changed);
        developer->addRow(row);
    }
    m_extraRow = new EntryRow(tr("Extra variables"), settings.extraEnv);
    m_extraRow->setPlaceholder(tr("NAME=value, separated by spaces"));
    m_extraRow->setSubtitle(tr("For experiments; docs/ENVIRONMENT.md lists every BB_* variable, and the log's "
                               "Environment: line names unknown ones"));
    connect(m_extraRow, &EntryRow::textEdited, this, &AdvancedPage::changed);
    developer->addRow(m_extraRow);
    addGroup(developer);

    SettingsGroup* launcher = new SettingsGroup(tr("Launcher"));
    m_themeRow = new ChoiceRow(tr("Theme"), {}, Choices::themes(), settings.theme);
    connect(m_themeRow, &ChoiceRow::valueChanged, this, [this](const QString& theme)
    {
        emit themeChanged(theme);
        emit changed();
    });
    launcher->addRow(m_themeRow);
    SettingRow* dataRow = new SettingRow(tr("Data folder"), PortPaths::dataDir());
    QToolButton* openData = Icons::button(Icons::Kind::FileManager, tr("Open in the file manager"), dataRow);
    dataRow->addSuffix(openData);
    connect(openData, &QToolButton::clicked, this, []()
    {
        UiHelpers::openFolder(PortPaths::dataDir());
    });
    launcher->addRow(dataRow);
    SettingRow* settingsRow = new SettingRow(tr("Launcher settings"), AppConfig::configFile());
    launcher->addRow(settingsRow);
    const QString portVersion = PortPaths::portVersion();
    SettingRow* aboutRow = new SettingRow(tr("bbpc %1").arg(QCoreApplication::applicationVersion()),
                                          portVersion.isEmpty()
                                              ? tr("A fork of bloodborne_pc (bbport) by deadinside28 and its contributors")
                                              : tr("A fork of bloodborne_pc (bbport %1) by deadinside28 and its contributors")
                                                    .arg(portVersion));
    QPushButton* aboutButton = new QPushButton(tr("About..."), aboutRow);
    aboutRow->addSuffix(aboutButton);
    connect(aboutButton, &QPushButton::clicked, this, [this]()
    {
        AboutDialog dialog(this);
        dialog.exec();
    });
    launcher->addRow(aboutRow);
    addGroup(launcher);
    finish();
}

void AdvancedPage::store(LauncherSettings& settings, PortIni&) const
{
    settings.drawPipe = m_pipeRow->value();
    settings.mangohud = m_mangohudRow->isOn();
    settings.saveLog = m_saveLogRow->isOn();
    settings.crashDiag = m_crashDiagRow->isOn();
    settings.frameStats = m_statsRow->isOn();
    settings.gpuProfile = m_profileRow->isOn();
    settings.vkValidation = m_validationRow->isOn();
    settings.extraEnv = m_extraRow->text().trimmed();
    settings.theme = m_themeRow->value();
}
