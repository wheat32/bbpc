#include "gamePage.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonObject>
#include <QLabel>
#include <QPushButton>
#include <QStandardPaths>
#include <QToolButton>

#include "core/choices.h"
#include "core/desktopShortcut.h"
#include "core/gameEnvironment.h"
#include "core/graphicsSettings.h"
#include "core/hostEnvironment.h"
#include "core/portPaths.h"
#include "process/portQueries.h"
#include "process/processRunner.h"
#include "widgets/choiceCombo.h"
#include "widgets/settingRows.h"
#include "widgets/uiHelpers.h"

namespace
{
constexpr int GAME_CHECK_TIMEOUT_MS = 60000;
constexpr int GPU_QUERY_TIMEOUT_MS = 10000;
constexpr int TRUST_TIMEOUT_MS = 5000;
const QString APP_ICON = QStringLiteral(":/assets/app-icon.svg");
const QString ICON_THEME_NAME = QStringLiteral("io.github.wheat32.bbpc");
constexpr double FPS_LIMIT_MIN = LauncherSettings::NO_FPS_LIMIT;
constexpr double FPS_LIMIT_MAX = 480;
constexpr double FPS_LIMIT_STEP = 1;
constexpr int FPS_LIMIT_DECIMALS = 0;

// Changes when eboot.bin or param.sfo changes: the check reads ~90 MB, so it runs once per state.
QString checkKey(const QString& gameDir)
{
    QString key = gameDir;
    for (const QString& file : { QStringLiteral("/eboot.bin"), QStringLiteral("/sce_sys/param.sfo") })
    {
        const QFileInfo info(gameDir + file);
        key += u'|' + QString::number(info.size()) + u':' + QString::number(info.lastModified().toMSecsSinceEpoch());
    }
    return key;
}

QString problemText(const QJsonObject& problem)
{
    const QString kind = problem.value(QStringLiteral("kind")).toString();
    const QString title = problem.value(QStringLiteral("title")).toString();
    const QString version = problem.value(QStringLiteral("version")).toString();
    if (kind == QStringLiteral("missing_update"))
    {
        return GamePage::tr("The 1.09 update is needed: copy the dumped 1.09 update into the game folder, replacing "
                            "files (found version %1)").arg(version);
    }
    if (kind == QStringLiteral("wrong_eboot"))
    {
        return GamePage::tr("eboot.bin is not from 1.09: copy eboot.bin from the dumped 1.09 update into the game "
                            "folder, replacing it");
    }
    if (kind == QStringLiteral("patched_eboot"))
    {
        return GamePage::tr("This eboot.bin has Lance McDonald's 60 fps patch baked in, which crashes the game in the "
                            "gestures menu: copy a clean eboot.bin from the dumped 1.09 update into the game folder, "
                            "replacing it (bbpc has its own 60 fps)");
    }
    if (kind == QStringLiteral("other_title"))
    {
        return GamePage::tr("Not a retail release of Bloodborne (found %1): Bloodborne from any region with update "
                            "1.09 is needed").arg(title);
    }
    if (kind == QStringLiteral("unreadable"))
    {
        return GamePage::tr("eboot.bin cannot be read as a decrypted PS4 executable: dump the game again");
    }
    if (kind == QStringLiteral("damaged_files"))
    {
        return GamePage::tr("The game files were damaged when they were extracted: the shaders do not unpack and the "
                            "game would hang while loading. Extract the game and the 1.09 update again with a fixed "
                            "tool (LibOrbisPkg's PkgTool on .NET 6 or newer leaves stale sectors)");
    }
    return GamePage::tr("The game check failed: %1").arg(kind);
}
} // namespace

GamePage::GamePage(const LauncherSettings& settings, const PortIni& ini, QWidget* parent)
    : LauncherPage(parent), m_gameDir(settings.gameDir), m_customUserDir(settings.userDir), m_savedGpu(settings.gpu)
{
    SettingsGroup* game = new SettingsGroup(tr("Game"), tr("Where the game and the saves are, and its languages"));

    m_gameRow = new SettingRow(tr("Game folder (CUSA03173)"));
    m_gameRow->setToolTip(tr("The unpacked game with the 1.09 update: the folder holding eboot.bin and dvdroot_ps4"));
    m_gameStatus = UiHelpers::statusIcon(m_gameRow);
    m_gameRow->addPrefix(m_gameStatus);
    QToolButton* chooseGame = Icons::button(Icons::Kind::FolderOpen, tr("Choose the folder with eboot.bin"), m_gameRow);
    QToolButton* openGame = Icons::button(Icons::Kind::FileManager, tr("Open in the file manager"), m_gameRow);
    m_gameRow->addSuffix(chooseGame);
    m_gameRow->addSuffix(openGame);
    connect(chooseGame, &QToolButton::clicked, this, &GamePage::chooseGameDir);
    connect(openGame, &QToolButton::clicked, this, [this]()
    {
        UiHelpers::openFolder(gameDir());
    });
    game->addRow(m_gameRow);

    // Saves and the shader cache: user/ in the data folder unless another is chosen.
    m_userRow = new SettingRow(tr("Saves folder"));
    m_userRow->setToolTip(tr("Saves, trophies and the shader cache. By default the user folder in the data folder"));
    m_userStatus = UiHelpers::statusIcon(m_userRow);
    m_userRow->addPrefix(m_userStatus);
    QToolButton* chooseUser = Icons::button(Icons::Kind::FolderOpen, tr("Choose the saves folder"), m_userRow);
    QToolButton* openUser = Icons::button(Icons::Kind::FileManager, tr("Open in the file manager"), m_userRow);
    m_userReset = Icons::button(Icons::Kind::Undo, tr("Back to the default folder"), m_userRow);
    m_userRow->addSuffix(chooseUser);
    m_userRow->addSuffix(openUser);
    m_userRow->addSuffix(m_userReset);
    connect(chooseUser, &QToolButton::clicked, this, &GamePage::chooseUserDir);
    connect(openUser, &QToolButton::clicked, this, [this]()
    {
        UiHelpers::openFolder(userDir());
    });
    connect(m_userReset, &QToolButton::clicked, this, [this]()
    {
        m_customUserDir.clear();
        showUserStatus();
        emit userDirChanged(userDir());
        emit changed();
    });
    game->addRow(m_userRow);

    m_languageRow = new ChoiceRow(tr("System language"),
                                  tr("The console language the game sees: it decides the menu language and the confirm "
                                     "button"),
                                  Choices::systemLanguages(), settings.language);
    connect(m_languageRow, &ChoiceRow::valueChanged, this, [this](const QString& language)
    {
        emit systemLanguageChanged(language);
        emit changed();
    });
    game->addRow(m_languageRow);
    m_textLanguageRow = new ChoiceRow(tr("Game text language"),
                                      tr("Any language the game's files have, whatever the copy's region and the save"),
                                      Choices::gameTextLanguages(), ini.value(QStringLiteral("game_text_language")));
    connect(m_textLanguageRow, &ChoiceRow::valueChanged, this, &GamePage::changed);
    game->addRow(m_textLanguageRow);
    m_voiceLanguageRow = new ChoiceRow(tr("Voice language"),
                                       tr("The voices from the game's files (when the copy has that language)"),
                                       Choices::gameVoiceLanguages(), ini.value(QStringLiteral("game_voice_language")));
    connect(m_voiceLanguageRow, &ChoiceRow::valueChanged, this, &GamePage::changed);
    game->addRow(m_voiceLanguageRow);

    // A menu entry and a desktop icon that start the game directly (--play).
    SettingRow* shortcutRow = new SettingRow(tr("Desktop shortcut"),
                                             tr("Starts the game at once, with the saved settings, patches and mods: a "
                                                "menu entry and an icon on the desktop"));
    QPushButton* shortcutButton = new QPushButton(tr("Create"), shortcutRow);
    shortcutRow->addSuffix(shortcutButton);
    connect(shortcutButton, &QPushButton::clicked, this, &GamePage::createShortcut);
    game->addRow(shortcutRow);
    addGroup(game);

    SettingsGroup* gpu = new SettingsGroup(tr("GPU"));
    m_gpuRow = new ChoiceRow(tr("GPU"), tr("On a laptop with two GPUs, pick the discrete one; applies when the game starts"));
    QToolButton* refreshGpus = Icons::button(Icons::Kind::Refresh, tr("Refresh the list"), m_gpuRow);
    m_gpuRow->addSuffix(refreshGpus);
    connect(refreshGpus, &QToolButton::clicked, this, &GamePage::fillGpus);
    connect(m_gpuRow, &ChoiceRow::valueChanged, this, [this](const QString& value)
    {
        m_savedGpu = value;
        emit changed();
    });
    gpu->addRow(m_gpuRow);
    addGroup(gpu);

    SettingsGroup* frames = new SettingsGroup(tr("Frame rate and latency"));
    m_fpsRow = new ChoiceRow(tr("Mode"), tr("Which frame rate patch the game gets (physics and animations stay right)"),
                             Choices::fpsModes(), settings.fpsMode);
    connect(m_fpsRow, &ChoiceRow::valueChanged, this, &GamePage::changed);
    frames->addRow(m_fpsRow);
    m_fpsLimitRow = new SpinRow(tr("FPS limit"), tr("0: no limit. A cap a little under what the PC reaches gives the "
                                                    "smoothest picture"),
                                FPS_LIMIT_MIN, FPS_LIMIT_MAX, FPS_LIMIT_STEP, FPS_LIMIT_DECIMALS, settings.fpsLimit);
    connect(m_fpsLimitRow, &SpinRow::valueChanged, this, &GamePage::changed);
    frames->addRow(m_fpsLimitRow);
    // NVIDIA Reflex or AMD Anti-Lag 2 (render/vk_low_latency); nothing changes on other drivers.
    m_frameGeneration = GraphicsSettings::frameGenerationChoice(ini.value(QStringLiteral("frame_generation")),
                                                                ini.value(QStringLiteral("frame_generation_multiplier")));
    m_lowLatencyRow = new ChoiceRow(tr("Low latency (NVIDIA Reflex / AMD Anti-Lag 2)"),
                                    tr("The game waits for the driver before each frame, so that frames do not pile up "
                                       "before the GPU: less input lag. NVIDIA: Reflex, driver 550 and newer; AMD and "
                                       "Intel: Anti-Lag 2 (Boost is the same as On). With DLSS frame generation always "
                                       "at least On"),
                                    Choices::lowLatency(),
                                    GraphicsSettings::lowLatency(ini.value(QStringLiteral("low_latency")), m_frameGeneration));
    connect(m_lowLatencyRow, &ChoiceRow::valueChanged, this, [this]()
    {
        enforceLowLatency();
        emit changed();
    });
    frames->addRow(m_lowLatencyRow);
    addGroup(frames);
    finish();

    m_gpuRow->combo()->setItems({ { tr("Auto"), QString(), {} } });
    fillGpus();
    showUserStatus();
    refreshGameStatus();
}

QString GamePage::gameDir() const
{
    return GameEnvironment::expandHome(m_gameDir);
}

bool GamePage::hasGame() const
{
    return m_gameDir.isEmpty() == false && QFileInfo(gameDir() + QStringLiteral("/eboot.bin")).isFile();
}

QString GamePage::userDir() const
{
    if (m_customUserDir.isEmpty() == false)
    {
        return GameEnvironment::expandHome(m_customUserDir);
    }
    return PortPaths::dataDir() + QStringLiteral("/user");
}

QString GamePage::systemLanguage() const
{
    return m_languageRow->value();
}

void GamePage::store(LauncherSettings& settings, PortIni& ini) const
{
    settings.gameDir = m_gameDir;
    settings.userDir = m_customUserDir;
    settings.language = m_languageRow->value();
    settings.gpu = m_savedGpu;
    settings.fpsMode = m_fpsRow->value();
    settings.fpsLimit = static_cast<int>(m_fpsLimitRow->value());
    ini.setValue(QStringLiteral("game_text_language"), m_textLanguageRow->value());
    ini.setValue(QStringLiteral("game_voice_language"), m_voiceLanguageRow->value());
    ini.setValue(QStringLiteral("low_latency"), GraphicsSettings::lowLatency(m_lowLatencyRow->value(), m_frameGeneration));
}

void GamePage::setFrameGeneration(const QString& choice)
{
    m_frameGeneration = choice;
    enforceLowLatency();
}

void GamePage::enforceLowLatency()
{
    // What is shown is what runs: the game keeps low latency on with DLSS Frame Generation.
    const QString effective = GraphicsSettings::lowLatency(m_lowLatencyRow->value(), m_frameGeneration);
    if (effective != m_lowLatencyRow->value())
    {
        m_lowLatencyRow->combo()->setValue(effective);
    }
}

void GamePage::createShortcut()
{
    const QStringList command = DesktopShortcut::playCommand(qEnvironmentVariable("APPIMAGE"),
                                                             QCoreApplication::applicationFilePath());
    // The launcher's own icon, by its path: the icon theme may not have it (an AppImage).
    QString icon = ICON_THEME_NAME;
    QFile resource(APP_ICON);
    const QString iconPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)
                           + QStringLiteral("/icons/hicolor/scalable/apps/") + DesktopShortcut::ICON_NAME;
    if (resource.open(QIODevice::ReadOnly) && DesktopShortcut::writeIcon(resource.readAll(), iconPath))
    {
        icon = iconPath;
    }
    const QStringList folders = DesktopShortcut::folders(QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation),
                                                         QStandardPaths::writableLocation(QStandardPaths::DesktopLocation),
                                                         QDir::homePath());
    const DesktopShortcut::Result result = DesktopShortcut::write(DesktopShortcut::entry(command, icon), folders);
    if (result.written.isEmpty())
    {
        emit toast(tr("Could not create the shortcut: %1").arg(result.error));
        return;
    }
    // GNOME starts a desktop file from the desktop only once it is trusted.
    const QString gio = QStandardPaths::findExecutable(QStringLiteral("gio"));
    QStringList places;
    for (const QString& path : result.written)
    {
        if (gio.isEmpty() == false)
        {
            ProcessRunner::run(gio, { QStringLiteral("set"), path, QStringLiteral("metadata::trusted"), QStringLiteral("true") },
                               HostEnvironment::host(), TRUST_TIMEOUT_MS, this, nullptr);
        }
        places.append(QFileInfo(path).absolutePath());
    }
    emit toast(result.error.isEmpty() ? tr("Shortcut created: %1").arg(places.join(QStringLiteral(", ")))
                                      : tr("Shortcut created: %1 (%2)").arg(places.join(QStringLiteral(", ")), result.error));
}

void GamePage::chooseGameDir()
{
    const QString chosen = UiHelpers::chooseFolder(this, tr("Game folder (with eboot.bin)"), gameDir());
    if (chosen.isEmpty()) return;
    m_gameDir = chosen;
    refreshGameStatus();
    emit changed();
}

void GamePage::chooseUserDir()
{
    const QString chosen = UiHelpers::chooseFolder(this, tr("Saves folder"), userDir());
    if (chosen.isEmpty()) return;
    m_customUserDir = chosen;
    showUserStatus();
    emit userDirChanged(userDir());
    emit changed();
}

void GamePage::refreshGameStatus()
{
    const bool found = hasGame();
    emit gameStatusChanged(found);
    if (found == false)
    {
        m_gameRow->setSubtitle(m_gameDir.isEmpty() ? tr("not chosen") : gameDir());
        UiHelpers::setStatusIcon(m_gameStatus, Icons::Kind::Warning, tr("No eboot.bin in the folder"));
        return;
    }
    // Other versions of the game start and then crash in its code: the check names the problem.
    const QString key = checkKey(gameDir());
    if (key == m_checkedKey)
    {
        showGameStatus(m_checkedProblem);
        return;
    }
    m_gameRow->setSubtitle(gameDir() + u'\n' + tr("Checking the game files..."));
    PortQueries::query({ QStringLiteral("game-check"), gameDir() }, this, [this, key](const QJsonObject& answer)
    {
        if (key != checkKey(gameDir())) return;
        QString problem;
        if (answer.contains(PortQueries::ERROR_KEY))
        {
            problem = tr("The game could not be checked: %1").arg(answer.value(PortQueries::ERROR_KEY).toString());
        }
        else if (answer.value(QStringLiteral("problem")).isObject())
        {
            problem = problemText(answer.value(QStringLiteral("problem")).toObject());
        }
        m_checkedKey = key;
        m_checkedProblem = problem;
        showGameStatus(problem);
    }, GAME_CHECK_TIMEOUT_MS);
}

void GamePage::showGameStatus(const QString& problem)
{
    if (problem.isEmpty())
    {
        m_gameRow->setSubtitle(gameDir());
        UiHelpers::setStatusIcon(m_gameStatus, Icons::Kind::Ok, tr("Bloodborne, version 1.09"));
        return;
    }
    m_gameRow->setSubtitle(gameDir() + u'\n' + problem);
    UiHelpers::setStatusIcon(m_gameStatus, Icons::Kind::Warning, problem);
}

void GamePage::showUserStatus()
{
    const QString path = userDir();
    const bool custom = m_customUserDir.isEmpty() == false;
    m_userRow->setSubtitle(custom ? path : tr("Default: %1").arg(path));
    const QDir saves(path + QStringLiteral("/savedata"));
    const bool found = saves.exists() && saves.isEmpty() == false;
    UiHelpers::setStatusIcon(m_userStatus, found ? Icons::Kind::Ok : Icons::Kind::New,
                             found ? tr("Saves found") : tr("No saves yet: the game will create them here"));
    m_userReset->setEnabled(custom);
}

void GamePage::fillGpus()
{
    PortQueries::capabilities({ QStringLiteral("--gpus") }, this, [this](const QString& output)
    {
        m_gpus = Capabilities::parseGpus(output);
        QString best;
        for (const Capabilities::Gpu& gpu : m_gpus)
        {
            if (gpu.best)
            {
                best = gpu.name;
            }
        }
        QList<ChoiceCombo::Item> items = { { best.isEmpty() ? tr("Auto") : tr("Auto (%1)").arg(best), QString(), {} } };
        QStringList names;
        for (const Capabilities::Gpu& gpu : m_gpus)
        {
            QString label = gpu.name;
            if (gpu.kind == QStringLiteral("integrated"))
            {
                label = tr("%1 (integrated)").arg(gpu.name);
            }
            else if (gpu.kind == QStringLiteral("cpu"))
            {
                label = tr("%1 (software, very slow)").arg(gpu.name);
            }
            items.append({ label, gpu.name, {} });
            names.append(gpu.name);
        }
        const QString saved = m_savedGpu;
        if (saved.isEmpty() == false && names.contains(saved) == false)
        {
            items.append({ tr("%1 (not found)").arg(saved), saved, {} });
        }
        m_gpuRow->combo()->setItems(items);
        m_gpuRow->combo()->setValue(saved);
        m_savedGpu = saved;
        emit gpusChanged(m_gpus);
    }, GPU_QUERY_TIMEOUT_MS);
}
