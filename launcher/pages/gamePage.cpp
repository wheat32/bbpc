#include "gamePage.h"

#include <QDateTime>
#include <QFileInfo>
#include <QJsonObject>
#include <QLabel>
#include <QToolButton>

#include "core/choices.h"
#include "core/gameEnvironment.h"
#include "core/portPaths.h"
#include "process/portQueries.h"
#include "widgets/choiceCombo.h"
#include "widgets/settingRows.h"
#include "widgets/uiHelpers.h"

namespace
{
constexpr int GAME_CHECK_TIMEOUT_MS = 60000;
constexpr int GPU_QUERY_TIMEOUT_MS = 10000;
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

GamePage::GamePage(const LauncherSettings& settings, const PortIni&, QWidget* parent)
    : LauncherPage(parent), m_gameDir(settings.gameDir), m_customUserDir(settings.userDir), m_savedGpu(settings.gpu)
{
    SettingsGroup* game = new SettingsGroup(tr("Game"));

    m_gameRow = new SettingRow(tr("Game folder (CUSA03173)"));
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

    m_languageRow = new ChoiceRow(tr("Game language"), tr("The language of the game's own text (all 21 it has)"),
                                  Choices::gameLanguages(), settings.language);
    connect(m_languageRow, &ChoiceRow::valueChanged, this, &GamePage::changed);
    game->addRow(m_languageRow);
    addGroup(game);

    SettingsGroup* mode = new SettingsGroup(tr("Mode"));
    m_memoryModelRow = new ChoiceRow(tr("Memory and translation model"),
                                     tr("The new one: the GPU works with the game's memory directly, as in a PC game, "
                                        "and graphics commands are translated rather than emulated. The old one: the "
                                        "memory model of 0.3 with all the fixes. A driver that fails the startup check "
                                        "gets the old one"),
                                     Choices::memoryModels(), settings.memoryModel);
    connect(m_memoryModelRow, &ChoiceRow::valueChanged, this, &GamePage::changed);
    mode->addRow(m_memoryModelRow);

    m_gpuRow = new ChoiceRow(tr("GPU"), tr("On a laptop with two GPUs, pick the discrete one; applies when the game starts"));
    QToolButton* refreshGpus = Icons::button(Icons::Kind::Refresh, tr("Refresh the list"), m_gpuRow);
    m_gpuRow->addSuffix(refreshGpus);
    connect(refreshGpus, &QToolButton::clicked, this, &GamePage::fillGpus);
    connect(m_gpuRow, &ChoiceRow::valueChanged, this, [this](const QString& value)
    {
        m_savedGpu = value;
        emit changed();
    });
    mode->addRow(m_gpuRow);
    addGroup(mode);

    SettingsGroup* frames = new SettingsGroup(tr("Frame rate"));
    m_fpsRow = new ChoiceRow(tr("Mode"), tr("Which frame rate patch to apply to the game"), Choices::fpsModes(),
                             settings.fpsMode);
    connect(m_fpsRow, &ChoiceRow::valueChanged, this, &GamePage::changed);
    frames->addRow(m_fpsRow);
    m_fpsLimitRow = new SpinRow(tr("FPS limit"), tr("0: no limit; set a number to cap the frame rate"), FPS_LIMIT_MIN,
                                FPS_LIMIT_MAX, FPS_LIMIT_STEP, FPS_LIMIT_DECIMALS, settings.fpsLimit);
    connect(m_fpsLimitRow, &SpinRow::valueChanged, this, &GamePage::changed);
    frames->addRow(m_fpsLimitRow);
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

void GamePage::store(LauncherSettings& settings, PortIni&) const
{
    settings.gameDir = m_gameDir;
    settings.userDir = m_customUserDir;
    settings.language = m_languageRow->value();
    settings.memoryModel = m_memoryModelRow->value();
    settings.gpu = m_savedGpu;
    settings.fpsMode = m_fpsRow->value();
    settings.fpsLimit = static_cast<int>(m_fpsLimitRow->value());
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
    }, GPU_QUERY_TIMEOUT_MS);
}
