#include "modsPage.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QToolButton>

#include "core/gameEnvironment.h"
#include "core/portPaths.h"
#include "process/portQueries.h"
#include "widgets/icons.h"
#include "widgets/settingRows.h"
#include "widgets/uiHelpers.h"

namespace
{
constexpr int LIST_TIMEOUT_MS = 30000;
constexpr int LOAD_EARLIER = -1;
constexpr int LOAD_LATER = 1;

QString modsProfilePath()
{
    return PortPaths::dataDir() + QStringLiteral("/mods.json");
}

QString patchesProfilePath()
{
    return PortPaths::dataDir() + QStringLiteral("/patches.json");
}
} // namespace

ModsPage::ModsPage(const LauncherSettings& settings, const PortIni&, QWidget* parent)
    : LauncherPage(parent), m_modsDir(settings.modsDir), m_patchesDir(settings.patchesDir)
{
    m_modsGroup = new SettingsGroup(tr("Mods"), tr("Extract each mod into its own folder (with dvdroot_ps4, or chr/, "
                                                   "parts/, ... directly). When files collide, the mod lower in the "
                                                   "list wins. Applied at start."));
    m_modsEnabledRow = new SwitchRow(tr("Load mods"), {}, settings.modsEnabled);
    connect(m_modsEnabledRow, &SwitchRow::toggled, this, &ModsPage::changed);
    m_modsGroup->addRow(m_modsEnabledRow);

    m_modsFolderRow = new SettingRow(tr("Mods folder"));
    QToolButton* chooseMods = Icons::button(Icons::Kind::FolderOpen, tr("Choose the mods folder"), m_modsFolderRow);
    QToolButton* openMods = Icons::button(Icons::Kind::FileManager, tr("Open the mods folder"), m_modsFolderRow);
    QToolButton* refreshModsButton = Icons::button(Icons::Kind::Refresh, tr("Refresh the list"), m_modsFolderRow);
    m_modsFolderRow->addSuffix(chooseMods);
    m_modsFolderRow->addSuffix(openMods);
    m_modsFolderRow->addSuffix(refreshModsButton);
    connect(chooseMods, &QToolButton::clicked, this, [this]()
    {
        const QString chosen = UiHelpers::chooseFolder(this, tr("Mods folder"), modsDir());
        if (chosen.isEmpty()) return;
        saveMods();
        m_modsDir = chosen;
        refreshMods();
        emit changed();
    });
    connect(openMods, &QToolButton::clicked, this, [this]()
    {
        UiHelpers::openFolder(modsDir());
    });
    connect(refreshModsButton, &QToolButton::clicked, this, [this]()
    {
        saveMods();
        refreshMods();
    });
    m_modsGroup->addRow(m_modsFolderRow);
    addGroup(m_modsGroup);

    m_patchesGroup = new SettingsGroup(tr("Third-party patches"),
                                       tr("shadPS4-format XML patches for version 01.09 from the patches folder. "
                                          "Applied at start."));
    m_patchesFolderRow = new SettingRow(tr("Patches folder"));
    QToolButton* choosePatches = Icons::button(Icons::Kind::FolderOpen, tr("Choose the patches folder"), m_patchesFolderRow);
    QToolButton* openPatches = Icons::button(Icons::Kind::FileManager, tr("Open the patches folder"), m_patchesFolderRow);
    QToolButton* refreshPatchesButton = Icons::button(Icons::Kind::Refresh, tr("Refresh the list"), m_patchesFolderRow);
    m_patchesFolderRow->addSuffix(choosePatches);
    m_patchesFolderRow->addSuffix(openPatches);
    m_patchesFolderRow->addSuffix(refreshPatchesButton);
    connect(choosePatches, &QToolButton::clicked, this, [this]()
    {
        const QString chosen = UiHelpers::chooseFolder(this, tr("Patches folder"), patchesDir());
        if (chosen.isEmpty()) return;
        savePatches();
        m_patchesDir = chosen;
        refreshPatches();
        emit changed();
    });
    connect(openPatches, &QToolButton::clicked, this, [this]()
    {
        UiHelpers::openFolder(patchesDir());
    });
    connect(refreshPatchesButton, &QToolButton::clicked, this, [this]()
    {
        savePatches();
        refreshPatches();
    });
    m_patchesGroup->addRow(m_patchesFolderRow);
    addGroup(m_patchesGroup);
    finish();

    refreshMods();
    refreshPatches();
}

void ModsPage::store(LauncherSettings& settings, PortIni&) const
{
    settings.modsEnabled = m_modsEnabledRow->isOn();
    settings.modsDir = m_modsDir;
    settings.patchesDir = m_patchesDir;
    saveMods();
    savePatches();
}

QString ModsPage::modsDir() const
{
    LauncherSettings settings;
    settings.modsDir = m_modsDir;
    return GameEnvironment::modsDir(settings, PortPaths::dataDir());
}

QString ModsPage::patchesDir() const
{
    LauncherSettings settings;
    settings.patchesDir = m_patchesDir;
    return GameEnvironment::patchesDir(settings, PortPaths::dataDir());
}

void ModsPage::refreshMods()
{
    const QString directory = modsDir();
    m_modsFolderRow->setSubtitle(directory);
    m_modsLoaded = false;
    PortQueries::query({ QStringLiteral("mods"), directory }, this, [this, directory](const QJsonObject& answer)
    {
        if (directory != modsDir()) return;
        m_modsLoaded = answer.contains(PortQueries::ERROR_KEY) == false;
        QStringList available;
        for (const QJsonValue& name : answer.value(QStringLiteral("mods")).toArray())
        {
            available.append(name.toString());
        }
        m_mods = Profiles::arrangeMods(available, Profiles::readJson(modsProfilePath()));
        showMods();
        if (answer.contains(PortQueries::ERROR_KEY))
        {
            m_modsFolderRow->setSubtitle(directory + u'\n' + answer.value(PortQueries::ERROR_KEY).toString());
        }
    }, LIST_TIMEOUT_MS);
}

void ModsPage::showMods()
{
    for (QWidget* row : m_modRows)
    {
        m_modsGroup->removeRow(row);
    }
    m_modRows.clear();
    for (int i = 0; i < m_mods.size(); ++i)
    {
        SwitchRow* row = new SwitchRow(m_mods.at(i).name, {}, m_mods.at(i).enabled);
        QToolButton* up = Icons::button(Icons::Kind::Up, tr("Load earlier"), row);
        QToolButton* down = Icons::button(Icons::Kind::Down, tr("Load later"), row);
        up->setEnabled(i > 0);
        down->setEnabled(i + 1 < m_mods.size());
        row->addSuffix(up);
        row->addSuffix(down);
        connect(row, &SwitchRow::toggled, this, [this, i](bool on)
        {
            m_mods[i].enabled = on;
            saveMods();
        });
        connect(up, &QToolButton::clicked, this, [this, i]()
        {
            moveMod(i, LOAD_EARLIER);
        });
        connect(down, &QToolButton::clicked, this, [this, i]()
        {
            moveMod(i, LOAD_LATER);
        });
        m_modsGroup->addRow(row);
        m_modRows.append(row);
    }
    m_modsFolderRow->setSubtitle(m_mods.isEmpty() ? tr("%1 (no mods)").arg(modsDir()) : modsDir());
}

void ModsPage::moveMod(int index, int direction)
{
    const int target = index + direction;
    if (target < 0 || target >= m_mods.size()) return;
    m_mods.swapItemsAt(index, target);
    saveMods();
    showMods();
}

void ModsPage::saveMods() const
{
    if (m_modsLoaded == false) return;
    Profiles::writeJson(modsProfilePath(), Profiles::modsJson(m_mods));
}

void ModsPage::refreshPatches()
{
    const QString directory = patchesDir();
    m_patchesFolderRow->setSubtitle(directory);
    PortQueries::query({ QStringLiteral("patches"), directory }, this, [this, directory](const QJsonObject& answer)
    {
        if (directory != patchesDir()) return;
        for (SwitchRow* row : m_patchRows)
        {
            m_patchesGroup->removeRow(row);
        }
        m_patchRows.clear();
        m_patches.clear();
        for (const QJsonValue& value : answer.value(QStringLiteral("patches")).toArray())
        {
            const QJsonObject patch = value.toObject();
            m_patches.append({ patch.value(QStringLiteral("key")).toString(), patch.value(QStringLiteral("name")).toString(),
                               patch.value(QStringLiteral("author")).toString(), patch.value(QStringLiteral("note")).toString(),
                               patch.value(QStringLiteral("default")).toBool(), false });
        }
        Profiles::applyPatchProfile(m_patches, Profiles::readJson(patchesProfilePath()));
        for (int i = 0; i < m_patches.size(); ++i)
        {
            const Profiles::Patch& patch = m_patches.at(i);
            QString subtitle = patch.key.section(u'/', 0, 0);
            if (patch.author.isEmpty() == false)
            {
                subtitle += QStringLiteral(" · ") + tr("Author: %1").arg(patch.author);
            }
            SwitchRow* row = new SwitchRow(patch.name, subtitle, patch.enabled);
            row->setToolTip(patch.note);
            connect(row, &SwitchRow::toggled, this, [this, i](bool on)
            {
                m_patches[i].enabled = on;
                savePatches();
            });
            m_patchesGroup->addRow(row);
            m_patchRows.append(row);
        }
        QString subtitle = m_patches.isEmpty() ? tr("%1 (no patches)").arg(directory) : directory;
        if (answer.contains(PortQueries::ERROR_KEY))
        {
            subtitle += u'\n' + answer.value(PortQueries::ERROR_KEY).toString();
        }
        m_patchesFolderRow->setSubtitle(subtitle);
    }, LIST_TIMEOUT_MS);
}

void ModsPage::savePatches() const
{
    const QString path = patchesProfilePath();
    Profiles::writeJson(path, Profiles::patchesJson(m_patches, Profiles::readJson(path)));
}
