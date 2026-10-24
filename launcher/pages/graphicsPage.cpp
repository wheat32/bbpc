#include "graphicsPage.h"

#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonObject>
#include <QMessageBox>
#include <QPushButton>
#include <QToolButton>
#include <cstdlib>

#include "core/choices.h"
#include "core/fsr411Build.h"
#include "core/portPaths.h"
#include "core/upscalerAssets.h"
#include "process/groupProcess.h"
#include "process/portQueries.h"
#include "widgets/choiceCombo.h"
#include "widgets/icons.h"
#include "widgets/settingRows.h"

namespace
{
constexpr int DLL_CHECK_TIMEOUT_MS = 30000;
// dll-info's status for a DLL the build can use (tools/fsr4cap/dll_info.py).
constexpr int DLL_FITS = 0;
constexpr double SHARPNESS_MIN = 0.0;
constexpr double SHARPNESS_MAX = 2.0;
constexpr double SHARPNESS_STEP = 0.05;
constexpr int SHARPNESS_DECIMALS = 2;
const QString FSR411 = QStringLiteral("fsr411");

QString dllFilter()
{
    return GraphicsPage::tr("DLL (*.dll *.DLL)");
}

QString onOff(bool on)
{
    return on ? QStringLiteral("1") : QStringLiteral("0");
}
} // namespace

GraphicsPage::GraphicsPage(const LauncherSettings& settings, const PortIni& ini, QWidget* parent)
    : LauncherPage(parent), m_dllDir(settings.fsr411DllDir)
{
    m_build = new GroupProcess(this);
    connect(m_build, &GroupProcess::lineReceived, this, [this](const QString& line)
    {
        emit logLine(line);
        const QString progress = Fsr411Build::progress(line);
        if (progress.isEmpty() == false)
        {
            updateFsr411Row(progress);
        }
    });
    connect(m_build, &GroupProcess::finished, this, &GraphicsPage::buildFinished);

    SettingsGroup* upscaler = new SettingsGroup(tr("Upscaler"),
                                                tr("Stored in bbport.ini; in game, change it in the menu (Insert or L3+R3)"));
    m_upscalerRow = new ChoiceRow(tr("Upscaler"), {}, Choices::upscalers(), ini.value(QStringLiteral("upscaler")));
    connect(m_upscalerRow, &ChoiceRow::valueChanged, this, [this]()
    {
        updateUpscalerStatus();
        emit changed();
    });
    upscaler->addRow(m_upscalerRow);

    // FSR 4.1.1 is built from the user's own AMD DLL (nothing of AMD's is shipped).
    m_fsr411Row = new SettingRow(tr("FSR 4.1.1 from your own AMD DLL"));
    m_fsr411Button = new QPushButton(m_fsr411Row);
    m_fsr411Row->addSuffix(m_fsr411Button);
    connect(m_fsr411Button, &QPushButton::clicked, this, [this]()
    {
        if (isBuilding())
        {
            cancelBuild();
            return;
        }
        chooseFsr411Dll();
    });
    m_fsr411Row->setVisible(QFileInfo(PortPaths::fsr4capDir() + QStringLiteral("/build_assets.sh")).isFile());
    upscaler->addRow(m_fsr411Row);

    // DLSS: NVIDIA's library, chosen like the FSR 4.1.1 DLL (a build with the DLSS SDK brings one).
    m_dlssRow = new SettingRow(tr("DLSS: NVIDIA's library"));
    QToolButton* chooseDlss = Icons::button(Icons::Kind::Document, tr("Choose a file..."), m_dlssRow);
    m_dlssReset = Icons::button(Icons::Kind::Undo, tr("Remove the chosen one"), m_dlssRow);
    m_dlssRow->addSuffix(chooseDlss);
    m_dlssRow->addSuffix(m_dlssReset);
    connect(chooseDlss, &QToolButton::clicked, this, &GraphicsPage::chooseDlssLibrary);
    connect(m_dlssReset, &QToolButton::clicked, this, &GraphicsPage::removeDlssLibrary);
    upscaler->addRow(m_dlssRow);

    m_presetRow = new ChoiceRow(tr("Preset"), {}, Choices::presets(), ini.value(QStringLiteral("preset")));
    connect(m_presetRow, &ChoiceRow::valueChanged, this, [this]()
    {
        updateUpscalerStatus();
        emit changed();
    });
    upscaler->addRow(m_presetRow);
    m_sharpenRow = new SwitchRow(tr("Sharpening (RCAS)"), {}, ini.value(QStringLiteral("sharpen")) == QStringLiteral("1"));
    connect(m_sharpenRow, &SwitchRow::toggled, this, &GraphicsPage::changed);
    upscaler->addRow(m_sharpenRow);
    m_sharpnessRow = new SpinRow(tr("Sharpness"), {}, SHARPNESS_MIN, SHARPNESS_MAX, SHARPNESS_STEP, SHARPNESS_DECIMALS,
                                 ini.value(QStringLiteral("sharpness")).toDouble());
    connect(m_sharpnessRow, &SpinRow::valueChanged, this, &GraphicsPage::changed);
    upscaler->addRow(m_sharpnessRow);
    m_motionRow = new SwitchRow(tr("Object motion vectors"), tr("Less ghosting on characters; costs about 10% FPS"),
                                ini.value(QStringLiteral("object_motion")) == QStringLiteral("1"));
    connect(m_motionRow, &SwitchRow::toggled, this, &GraphicsPage::changed);
    upscaler->addRow(m_motionRow);
    m_showFpsRow = new SwitchRow(tr("Show FPS"), {}, ini.value(QStringLiteral("show_fps")) == QStringLiteral("1"));
    connect(m_showFpsRow, &SwitchRow::toggled, this, &GraphicsPage::changed);
    upscaler->addRow(m_showFpsRow);
    addGroup(upscaler);

    SettingsGroup* effects = new SettingsGroup(tr("Game effects"), tr("Game patches, applied at start"));
    m_lodRow = new ChoiceRow(tr("Model detail"), {}, Choices::modelDetail(), ini.value(QStringLiteral("model_lod")));
    connect(m_lodRow, &ChoiceRow::valueChanged, this, &GraphicsPage::changed);
    effects->addRow(m_lodRow);
    for (const Choices::Effect& effect : Choices::effects())
    {
        const QString key = QString::fromLatin1(effect.key);
        const QString fallback = onOff(effect.defaultOn);
        SwitchRow* row = new SwitchRow(Choices::title(effect), Choices::note(effect),
                                       ini.value(key, fallback) == QStringLiteral("1"));
        connect(row, &SwitchRow::toggled, this, &GraphicsPage::changed);
        m_effectRows.insert(key, row);
        effects->addRow(row);
    }
    // The title screen's "play online / play offline": the port has no PSN.
    m_skipNetworkRow = new SwitchRow(tr("Skip the \"play online / offline\" choice"),
                                     tr("The game opens the main menu offline at once (a game patch)"),
                                     settings.skipNetworkChoice);
    connect(m_skipNetworkRow, &SwitchRow::toggled, this, &GraphicsPage::changed);
    effects->addRow(m_skipNetworkRow);
    addGroup(effects);
    finish();

    m_outputResolution = ini.value(QStringLiteral("output_res"));
    updateFsr411Row();
    updateUpscalerStatus();
}

void GraphicsPage::store(LauncherSettings& settings, PortIni& ini) const
{
    settings.skipNetworkChoice = m_skipNetworkRow->isOn();
    settings.fsr411DllDir = m_dllDir;
    ini.setValue(QStringLiteral("upscaler"), m_upscalerRow->value());
    ini.setValue(QStringLiteral("preset"), m_presetRow->value());
    ini.setValue(QStringLiteral("sharpen"), onOff(m_sharpenRow->isOn()));
    ini.setValue(QStringLiteral("sharpness"), QString::number(m_sharpnessRow->value(), 'f', SHARPNESS_DECIMALS));
    ini.setValue(QStringLiteral("object_motion"), onOff(m_motionRow->isOn()));
    ini.setValue(QStringLiteral("show_fps"), onOff(m_showFpsRow->isOn()));
    ini.setValue(QStringLiteral("model_lod"), m_lodRow->value());
    for (auto it = m_effectRows.cbegin(); it != m_effectRows.cend(); ++it)
    {
        ini.setValue(it.key(), onOff(it.value()->isOn()));
    }
}

bool GraphicsPage::isBuilding() const
{
    return m_build->isRunning();
}

void GraphicsPage::cancelBuild()
{
    if (isBuilding() == false) return;
    m_buildCanceled = true;
    m_build->stop();
}

void GraphicsPage::setOutputResolution(const QString& output)
{
    m_outputResolution = output;
    updateUpscalerStatus();
}

void GraphicsPage::setUserDir(const QString& userDir)
{
    m_userDir = userDir;
    updateDlssRow();
}

void GraphicsPage::setGameRunning(bool running)
{
    m_gameRunning = running;
    updateFsr411Row();
}

void GraphicsPage::updateUpscalerStatus()
{
    const QString value = m_upscalerRow->value();
    QString hint;
    if (value == QStringLiteral("fsr4"))
    {
        const bool found = QFileInfo(PortPaths::portDir() + QStringLiteral("/fsr4_shaders")).isDir();
        hint = found ? tr("Assets found") : tr("No assets: run tools/fetch_fsr4_assets.sh");
    }
    else if (value == FSR411)
    {
        const QString directory = UpscalerAssets::fsr411Dir(PortPaths::portDir(), PortPaths::dataDir());
        const QString problem = UpscalerAssets::fsr411Problem(directory, m_outputResolution, m_presetRow->value().toInt());
        hint = problem.isEmpty()
            ? tr("Assets for the selected mode found")
            : tr("%1. Build FSR 4.1.1 from your own DLL with the \"Choose DLL...\" button below").arg(problem);
    }
    else if (value == QStringLiteral("dlss"))
    {
        // The game looks for the bridge and NVIDIA's library next to bb-probe (vk_dlss.cpp).
        const QString binaries = PortPaths::binariesDir();
        const bool found = QFileInfo(binaries + QStringLiteral("/libbbport_dlss.so")).isFile()
            && (UpscalerAssets::ngxLibraries(m_userDir + QStringLiteral("/dlss")).isEmpty() == false
                || UpscalerAssets::ngxLibraries(binaries).isEmpty() == false);
        hint = found ? tr("DLSS found (needs an NVIDIA RTX GPU)")
                     : tr("DLSS not found: choose NVIDIA's library below (a build with the DLSS bridge is needed too). "
                          "The game will use FSR 3.1");
    }
    else if (value == QStringLiteral("taa"))
    {
        hint = tr("Anti-aliasing at the output resolution, no FSR model");
    }
    m_presetRow->setEnabled(value != QStringLiteral("taa") && value != QStringLiteral("off"));
    m_sharpenRow->setEnabled(value != QStringLiteral("off"));
    m_sharpnessRow->setEnabled(value != QStringLiteral("off"));
    m_upscalerRow->setBaseSubtitle(hint);
}

void GraphicsPage::updateFsr411Row(const QString& progress)
{
    const bool building = isBuilding();
    m_fsr411Button->setText(building ? tr("Cancel") : tr("Choose DLL..."));
    m_fsr411Button->setEnabled(building || m_gameRunning == false);
    if (progress.isEmpty() == false)
    {
        m_fsr411Row->setSubtitle(progress);
    }
    else if (building == false)
    {
        m_fsr411Row->setSubtitle(tr("Build FSR 4.1.1 from amd_fidelityfx_upscaler_dx12.dll 4.1.x (OptiScaler: the "
                                    "FSR4_LATEST folder, or from a game with FSR 4.1). Needs GE-Proton 10+, Proton "
                                    "Experimental or Proton-CachyOS; 2-5 minutes (RDNA4: twice as long, with the FP8 "
                                    "variant)"));
    }
}

void GraphicsPage::updateDlssRow()
{
    const QFileInfoList own = UpscalerAssets::ngxLibraries(m_userDir + QStringLiteral("/dlss"));
    const QFileInfoList bundled = UpscalerAssets::ngxLibraries(PortPaths::binariesDir());
    const bool bridge = QFileInfo(PortPaths::binariesDir() + QStringLiteral("/libbbport_dlss.so")).isFile();
    QString text;
    if (own.isEmpty() == false)
    {
        text = tr("Your own: %1").arg(UpscalerAssets::ngxVersion(own.last()));
    }
    else if (bundled.isEmpty() == false)
    {
        text = tr("From this build: %1").arg(UpscalerAssets::ngxVersion(bundled.last()));
    }
    else
    {
        text = tr("None: choose libnvidia-ngx-dlss.so.* from NVIDIA's DLSS SDK (%1)").arg(UpscalerAssets::DLSS_SDK_URL);
    }
    if (bridge == false)
    {
        text += u'\n' + tr("This build has no DLSS bridge (libbbport_dlss.so): DLSS will not work");
    }
    m_dlssRow->setSubtitle(text);
    m_dlssReset->setEnabled(own.isEmpty() == false);
    updateUpscalerStatus();
}

QString GraphicsPage::chooseDll(const QString& title)
{
    const QString start = QFileInfo(m_dllDir).isDir() ? m_dllDir : QDir::homePath();
    return QFileDialog::getOpenFileName(this, title, start, dllFilter());
}

void GraphicsPage::chooseFsr411Dll()
{
    const QString upscaler = chooseDll(tr("AMD upscaler DLL (amd_fidelityfx_upscaler_dx12.dll)"));
    if (upscaler.isEmpty()) return;
    m_dllDir = QFileInfo(upscaler).absolutePath();
    emit changed();
    inspectDll(upscaler, {});
}

void GraphicsPage::inspectDll(const QString& upscaler, const QString& loader)
{
    QStringList arguments = { QStringLiteral("dll-info"), upscaler };
    if (loader.isEmpty() == false)
    {
        arguments.append(loader);
    }
    m_fsr411Button->setEnabled(false);
    m_fsr411Row->setSubtitle(tr("Checking the DLL..."));
    PortQueries::query(arguments, this, [this, upscaler, loader](const QJsonObject& answer)
    {
        updateFsr411Row();
        if (answer.contains(PortQueries::ERROR_KEY))
        {
            QMessageBox::warning(this, tr("The DLL could not be checked"), answer.value(PortQueries::ERROR_KEY).toString());
            return;
        }
        const int status = answer.value(QStringLiteral("status")).toInt();
        const QJsonObject details = answer.value(QStringLiteral("details")).toObject();
        const QString kind = details.value(QStringLiteral("kind")).toString();
        const QString version = details.value(QStringLiteral("version")).toString(QStringLiteral("?"));
        const QString name = QFileInfo(upscaler).fileName();
        if (status == answer.value(QStringLiteral("unsupported")).toInt())
        {
            QString body;
            if (kind == QStringLiteral("fsr4_0"))
            {
                body = tr("%1: FSR %2. This is AMD's official FSR 4.0.x: it is enabled on RDNA4 GPUs (RX 9000) only and "
                          "does not start under Proton on other GPUs, so it cannot be recorded.\n\nFSR 4.1.x is needed: "
                          "for example FSR4_LATEST from OptiScaler, or the DLL of a game with FSR 4.1.").arg(name, version);
            }
            else if (kind == QStringLiteral("not_fsr4"))
            {
                body = tr("%1: this DLL has no FSR 4 inside. amd_fidelityfx_upscaler_dx12.dll version 4.1.x is needed.").arg(name);
            }
            else
            {
                QStringList models;
                for (const QJsonValue& model : details.value(QStringLiteral("models")).toArray())
                {
                    models.append(model.toString());
                }
                body = tr("%1: version %2, FSR 4 models: %3.\n\nbbpc replays AMD's official FSR 4.1.x only (model "
                          "v07_fp8_no_scale): for example FSR4_LATEST from OptiScaler, or "
                          "amd_fidelityfx_upscaler_dx12.dll from a game with FSR 4.1. Community builds (4.0.2b and the "
                          "like) are laid out differently and are not supported yet.")
                           .arg(name, version, models.isEmpty() ? tr("none") : models.join(QStringLiteral(", ")));
            }
            QMessageBox::warning(this, tr("This DLL does not fit"), body);
            return;
        }
        if (status == answer.value(QStringLiteral("no_loader")).toInt())
        {
            if (kind == QStringLiteral("loader_is_upscaler"))
            {
                QMessageBox::warning(this, tr("This is not the loader"),
                                     tr("%1 is the upscaler DLL. The loader is called amd_fidelityfx_loader_dx12.dll "
                                        "(amd_fidelityfx_dx12.dll in OptiScaler) and lies in the same game folder as "
                                        "the upscaler DLL.").arg(QFileInfo(loader).fileName()));
                return;
            }
            if (loader.isEmpty() == false)
            {
                QMessageBox::warning(this, tr("The loader does not fit"),
                                     tr("%1: version %2. A FidelityFX loader 2.x is needed, from the same game as the "
                                        "upscaler DLL.").arg(QFileInfo(loader).fileName(),
                                                             details.value(QStringLiteral("loader_version")).toString(QStringLiteral("?"))));
                return;
            }
            // A DLL without the FidelityFX API exports (AMD's own export it: no loader needed) and
            // no loader next to it or one folder up: ask for it.
            emit toast(tr("No loader next to the DLL: choose amd_fidelityfx_loader_dx12.dll from the game's folder"));
            const QString chosen = chooseDll(tr("FidelityFX loader (amd_fidelityfx_loader_dx12.dll or amd_fidelityfx_dx12.dll)"));
            if (chosen.isEmpty()) return;
            inspectDll(upscaler, chosen);
            return;
        }
        if (status != DLL_FITS)
        {
            QMessageBox::warning(this, tr("This DLL does not fit"), answer.value(QStringLiteral("problem")).toString());
            return;
        }
        startBuild(upscaler, details.value(QStringLiteral("loader")).toString());
    }, DLL_CHECK_TIMEOUT_MS);
}

void GraphicsPage::startBuild(const QString& upscaler, const QString& loader)
{
    if (m_gameRunning) return;
    const Fsr411Build::Command command = Fsr411Build::command(upscaler, loader);
    m_buildCanceled = false;
    if (m_build->start(command.program, command.arguments, command.env, command.workingDirectory) == false)
    {
        emit toast(tr("Could not start: %1").arg(m_build->errorString()));
        return;
    }
    emit logLine(QString());
    emit logLine(tr("--- building FSR 4.1.1 from %1 ---").arg(upscaler));
    updateFsr411Row(tr("Preparing..."));
    emit buildRunningChanged(true);
}

void GraphicsPage::buildFinished(int exitCode)
{
    const bool canceled = m_buildCanceled;
    m_buildCanceled = false;
    updateFsr411Row();
    emit buildRunningChanged(false);
    emit logLine(tr("--- FSR 4.1.1 build finished (exit code %1) ---").arg(exitCode));
    if (exitCode == EXIT_SUCCESS)
    {
        m_upscalerRow->combo()->setValue(FSR411);
        updateUpscalerStatus();
        emit changed();
        emit toast(tr("FSR 4.1.1 built and selected"));
        return;
    }
    if (canceled)
    {
        emit toast(tr("FSR 4.1.1 build canceled"));
        return;
    }
    QMessageBox::warning(this, tr("FSR 4.1.1 was not built"), Fsr411Build::errorMessage(exitCode));
}

void GraphicsPage::chooseDlssLibrary()
{
    const QString path = QFileDialog::getOpenFileName(this, tr("NVIDIA's DLSS library (libnvidia-ngx-dlss.so.*)"),
                                                      QDir::homePath(),
                                                      tr("libnvidia-ngx-dlss.so.*, nvngx_dlss.dll (libnvidia-ngx-dlss.so* *.dll *.DLL)"));
    if (path.isEmpty()) return;
    installDlssLibrary(path);
}

void GraphicsPage::installDlssLibrary(const QString& path)
{
    const QFileInfo file(path);
    if (file.suffix().compare(QStringLiteral("dll"), Qt::CaseInsensitive) == 0)
    {
        QMessageBox::warning(this, tr("A Linux library is needed"),
                             tr("%1 is the Windows DLSS library; NVIDIA's Linux driver does not load it. "
                                "libnvidia-ngx-dlss.so.<version> from NVIDIA's DLSS SDK is needed: %2")
                                 .arg(file.fileName(), UpscalerAssets::DLSS_SDK_URL));
        return;
    }
    if (file.fileName().startsWith(UpscalerAssets::NGX_PREFIX) == false || UpscalerAssets::isElf(path) == false)
    {
        QMessageBox::warning(this, tr("This is not the DLSS library"),
                             tr("libnvidia-ngx-dlss.so.<version> from NVIDIA's DLSS SDK is needed: %1").arg(UpscalerAssets::DLSS_SDK_URL));
        return;
    }
    const QString target = m_userDir + QStringLiteral("/dlss");
    QDir().mkpath(target);
    for (const QFileInfo& old : UpscalerAssets::ngxLibraries(target))
    {
        QFile::remove(old.absoluteFilePath());
    }
    if (QFile::copy(path, target + u'/' + file.fileName()) == false)
    {
        QMessageBox::warning(this, tr("Could not copy the library"), tr("Could not copy %1 to %2").arg(path, target));
        return;
    }
    emit toast(tr("DLSS %1: ready").arg(UpscalerAssets::ngxVersion(file)));
    updateDlssRow();
}

void GraphicsPage::removeDlssLibrary()
{
    for (const QFileInfo& old : UpscalerAssets::ngxLibraries(m_userDir + QStringLiteral("/dlss")))
    {
        QFile::remove(old.absoluteFilePath());
    }
    updateDlssRow();
}
