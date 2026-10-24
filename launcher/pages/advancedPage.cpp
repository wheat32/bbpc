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
    SettingsGroup* performance = new SettingsGroup(tr("Performance"));
    m_pipeRow = new ChoiceRow(tr("Two-stage GPU pipeline"), tr("20-30% faster; turn it off if the game is unstable"),
                              Choices::drawPipe(), settings.drawPipe);
    m_readbacksRow = new ChoiceRow(tr("GPU data readbacks by the CPU"), {}, Choices::readbacks(), settings.readbacks);
    m_preuploadRow = new ChoiceRow(tr("Background pre-upload into VRAM"), tr("Fewer hitches when areas stream in"),
                                   Choices::preupload(), settings.preupload);
    for (ChoiceRow* row : { m_pipeRow, m_readbacksRow, m_preuploadRow })
    {
        connect(row, &ChoiceRow::valueChanged, this, &AdvancedPage::changed);
        performance->addRow(row);
    }
    addGroup(performance);

    SettingsGroup* developer = new SettingsGroup(tr("Developer"));
    m_mangohudRow = new SwitchRow(QStringLiteral("MangoHud"), tr("Its Vulkan layer must be installed (Arch: mangohud)"),
                                  settings.mangohud);
    m_saveLogRow = new SwitchRow(tr("Save the log and statistics to a file"),
                                 tr("Into the logs folder of the data folder: to look into stutters and crashes"),
                                 settings.saveLog);
    m_crashDiagRow = new SwitchRow(tr("Crash diagnostics"),
                                   tr("Checks the game's heap and logs writes into its memory; a little slower"),
                                   settings.crashDiag);
    m_as03Row = new SwitchRow(tr("Synchronization as in 0.3"),
                              tr("For finding regressions: the old memory model and copy waits as released in 0.3"),
                              settings.as03);
    m_statsRow = new SwitchRow(tr("Frame statistics in the log"), QStringLiteral("BB_FRAME_STATS"), settings.frameStats);
    m_profileRow = new SwitchRow(tr("GPU profile in the log"), QStringLiteral("BB_GPU_PROFILE"), settings.gpuProfile);
    m_validationRow = new SwitchRow(tr("Vulkan validation layers"), tr("Much slower"), settings.vkValidation);
    for (SwitchRow* row : { m_mangohudRow, m_saveLogRow, m_crashDiagRow, m_as03Row, m_statsRow, m_profileRow, m_validationRow })
    {
        connect(row, &SwitchRow::toggled, this, &AdvancedPage::changed);
        developer->addRow(row);
    }
    m_extraRow = new EntryRow(tr("Extra variables"), settings.extraEnv);
    m_extraRow->setPlaceholder(tr("NAME=value, separated by spaces"));
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
    SettingRow* aboutRow = new SettingRow(tr("bbpc %1").arg(QCoreApplication::applicationVersion()),
                                          tr("A fork of bloodborne_pc (bbport) by deadinside28 and its contributors"));
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
    settings.readbacks = m_readbacksRow->value();
    settings.preupload = m_preuploadRow->value();
    settings.mangohud = m_mangohudRow->isOn();
    settings.saveLog = m_saveLogRow->isOn();
    settings.crashDiag = m_crashDiagRow->isOn();
    settings.as03 = m_as03Row->isOn();
    settings.frameStats = m_statsRow->isOn();
    settings.gpuProfile = m_profileRow->isOn();
    settings.vkValidation = m_validationRow->isOn();
    settings.extraEnv = m_extraRow->text().trimmed();
    settings.theme = m_themeRow->value();
}
