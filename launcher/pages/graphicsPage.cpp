#include "graphicsPage.h"

#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonObject>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalBlocker>
#include <QToolButton>
#include <cstdlib>

#include "core/choices.h"
#include "core/fsr411Build.h"
#include "core/graphicsSettings.h"
#include "core/outputSizes.h"
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
const QString SSAO_EFFECT = QStringLiteral("effect_ssao");

QString dllFilter()
{
    return GraphicsPage::tr("DLL (*.dll *.DLL)");
}

QString describe(const QString& description, const QString& hint)
{
    if (hint.isEmpty())
    {
        return description;
    }
    return description.isEmpty() ? hint : description + u'\n' + hint;
}

QString onOff(bool on)
{
    return on ? QStringLiteral("1") : QStringLiteral("0");
}
} // namespace

GraphicsPage::GraphicsPage(const LauncherSettings& settings, const PortIni& ini, QWidget* parent)
    : LauncherPage(parent), m_dllDir(settings.fsr411DllDir), m_fsr411Dll(settings.fsr411Dll),
      m_frameGenerationMultiplier(ini.value(QStringLiteral("frame_generation_multiplier")))
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
                                                tr("Stored in bbport.ini; all of it can be changed in the game's menu "
                                                   "too (Insert, or hold L3+R3)"));
    m_upscalerRow = new ChoiceRow(tr("Upscaler"), {}, Choices::upscalers(), ini.value(QStringLiteral("upscaler")));
    connect(m_upscalerRow, &ChoiceRow::valueChanged, this, [this]()
    {
        updateUpscalerStatus();
        emit changed();
    });
    upscaler->addRow(m_upscalerRow);

    // Frame generation (the game's menu has it too): FSR 3.1 on any GPU, DLSS where an NVIDIA GPU is.
    m_frameGenerationRow = new ChoiceRow(tr("Frame generation"), {});
    m_frameGenerationRow->combo()->setChoices(Choices::frameGenerations());
    m_frameGenerationRow->combo()->setValue(GraphicsSettings::frameGenerationChoice(
        ini.value(QStringLiteral("frame_generation")), m_frameGenerationMultiplier));
    connect(m_frameGenerationRow, &ChoiceRow::valueChanged, this, [this](const QString& choice)
    {
        updateFrameGenerationHint();
        emit frameGenerationChanged(choice);
        emit changed();
    });
    upscaler->addRow(m_frameGenerationRow);

    m_presetRow = new ChoiceRow(tr("Preset"), tr("How much smaller the scene is than the output per side. A smaller "
                                                 "scene: more FPS, a softer picture"),
                                Choices::presets(), ini.value(QStringLiteral("preset")));
    connect(m_presetRow, &ChoiceRow::valueChanged, this, [this]()
    {
        updateUpscalerStatus();
        emit changed();
    });
    upscaler->addRow(m_presetRow);
    m_sharpenRow = new SwitchRow(tr("Sharpening (RCAS)"),
                                 tr("Sharpens after the upscaler; without one, over the game's AA or FXAA. Nearly free"),
                                 ini.value(QStringLiteral("sharpen")) == QStringLiteral("1"));
    upscaler->addRow(m_sharpenRow);
    m_sharpnessRow = new SpinRow(tr("Sharpness"), tr("0: unchanged, 1: AMD's maximum, above: stronger"), SHARPNESS_MIN,
                                 SHARPNESS_MAX, SHARPNESS_STEP, SHARPNESS_DECIMALS,
                                 ini.value(QStringLiteral("sharpness")).toDouble());
    connect(m_sharpnessRow, &SpinRow::valueChanged, this, &GraphicsPage::changed);
    upscaler->addRow(m_sharpnessRow);
    connect(m_sharpenRow, &SwitchRow::toggled, this, [this](bool on)
    {
        m_sharpnessRow->setEnabled(on);
        emit changed();
    });
    m_sharpnessRow->setEnabled(m_sharpenRow->isOn());
    m_motionRow = new SwitchRow(tr("Object motion vectors"),
                                tr("Less ghosting behind characters with temporal upscalers. Costs about 6% FPS; FSR 1 "
                                   "does not use them"),
                                ini.value(QStringLiteral("object_motion")) == QStringLiteral("1"));
    connect(m_motionRow, &SwitchRow::toggled, this, &GraphicsPage::changed);
    upscaler->addRow(m_motionRow);
    addGroup(upscaler);

    SettingsGroup* quality = new SettingsGroup(tr("Image quality"));
    m_antiAliasingRow = new ChoiceRow(tr("Anti-aliasing"), tr("Smoothing of jagged edges"), Choices::antiAliasing(),
                                      GraphicsSettings::antiAliasing(ini));
    connect(m_antiAliasingRow, &ChoiceRow::valueChanged, this, &GraphicsPage::changed);
    quality->addRow(m_antiAliasingRow);
    m_textureRow = new ChoiceRow(tr("Texture quality"),
                                 tr("The size of large textures in VRAM (from the next area). For GPUs with 2-4 GB"),
                                 Choices::textureQuality(), ini.value(QStringLiteral("texture_quality")));
    connect(m_textureRow, &ChoiceRow::valueChanged, this, &GraphicsPage::changed);
    quality->addRow(m_textureRow);
    m_lodRow = new ChoiceRow(tr("Model detail"), tr("The level of detail of distant models (after a restart)"),
                             Choices::modelDetail(), ini.value(QStringLiteral("model_lod")));
    connect(m_lodRow, &ChoiceRow::valueChanged, this, &GraphicsPage::changed);
    quality->addRow(m_lodRow);
    m_vbaoRow = new SwitchRow(tr("GT-VBAO occlusion"),
                              tr("A more accurate ambient occlusion in place of the game's SSAO (needs SSAO on); about "
                                 "7% FPS"),
                              ini.value(QStringLiteral("vbao")) == QStringLiteral("1"));
    connect(m_vbaoRow, &SwitchRow::toggled, this, &GraphicsPage::changed);
    quality->addRow(m_vbaoRow);
    addGroup(quality);

    SettingsGroup* libraries = new SettingsGroup(tr("AMD and NVIDIA libraries"),
                                                 tr("Your own AMD and NVIDIA files: the port does not ship them"));
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
    const bool canBuild = QFileInfo(PortPaths::fsr4capDir() + QStringLiteral("/build_assets.sh")).isFile();
    m_fsr411Row->setVisible(canBuild);
    libraries->addRow(m_fsr411Row);

    // FSR 4.1.2's models (INT8 RDNA2, INT8 Lite) from AMD's driver, also the user's own.
    m_fsr412Row = new SettingRow(tr("FSR 4.1.2 from AMD's driver"));
    m_fsr412Button = new QPushButton(m_fsr412Row);
    m_fsr412Row->addSuffix(m_fsr412Button);
    connect(m_fsr412Button, &QPushButton::clicked, this, [this]()
    {
        if (isBuilding())
        {
            cancelBuild();
            return;
        }
        chooseDriver();
    });
    m_fsr412Row->setVisible(canBuild && QFileInfo(PortPaths::fsr4capDir() + QStringLiteral("/driver_model.py")).isFile());
    libraries->addRow(m_fsr412Row);

    // DLSS: NVIDIA's libraries, chosen like the FSR 4.1.1 DLL (a build with the DLSS SDK brings them).
    m_dlssRow = new SettingRow(tr("DLSS: NVIDIA's libraries"));
    QToolButton* chooseDlss = Icons::button(Icons::Kind::Document, tr("Choose a file..."), m_dlssRow);
    m_dlssReset = Icons::button(Icons::Kind::Undo, tr("Remove the chosen ones"), m_dlssRow);
    m_dlssRow->addSuffix(chooseDlss);
    m_dlssRow->addSuffix(m_dlssReset);
    connect(chooseDlss, &QToolButton::clicked, this, &GraphicsPage::chooseDlssLibrary);
    connect(m_dlssReset, &QToolButton::clicked, this, &GraphicsPage::removeDlssLibrary);
    libraries->addRow(m_dlssRow);
    addGroup(libraries);

    SettingsGroup* effects = new SettingsGroup(tr("Game effects"),
                                               tr("The game's effects and patches, applied at start (most of the effects "
                                                  "also change live in the game's menu)"));
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
    // GT-VBAO replaces the game's SSAO: without it there is nothing to replace.
    SwitchRow* ssao = m_effectRows.value(SSAO_EFFECT);
    if (ssao != nullptr)
    {
        connect(ssao, &SwitchRow::toggled, m_vbaoRow, &SwitchRow::setEnabled);
        m_vbaoRow->setEnabled(ssao->isOn());
    }
    // The title screen's "play online / play offline": the port has no PSN.
    m_skipNetworkRow = new SwitchRow(tr("Skip the \"play online / offline\" choice"),
                                     tr("The game opens its main menu offline at once (a game patch). Turn it off for "
                                        "online play"),
                                     settings.skipNetworkChoice);
    connect(m_skipNetworkRow, &SwitchRow::toggled, this, &GraphicsPage::changed);
    effects->addRow(m_skipNetworkRow);
    addGroup(effects);
    finish();

    m_outputResolution = OutputSizes::format(OutputSizes::DEFAULT_SIZE);
    updateFrameGenerations();
    updateFsr411Row();
    updateUpscalerStatus();
}

void GraphicsPage::store(LauncherSettings& settings, PortIni& ini) const
{
    settings.skipNetworkChoice = m_skipNetworkRow->isOn();
    settings.fsr411DllDir = m_dllDir;
    settings.fsr411Dll = m_fsr411Dll;
    ini.setValue(QStringLiteral("upscaler"), m_upscalerRow->value());
    const GraphicsSettings::FrameGenerationKeys frames = GraphicsSettings::frameGenerationKeys(m_frameGenerationRow->value(),
                                                                                              m_frameGenerationMultiplier);
    ini.setValue(QStringLiteral("frame_generation"), frames.frameGeneration);
    ini.setValue(QStringLiteral("frame_generation_multiplier"), frames.multiplier);
    ini.setValue(QStringLiteral("preset"), m_presetRow->value());
    ini.setValue(QStringLiteral("sharpen"), onOff(m_sharpenRow->isOn()));
    ini.setValue(QStringLiteral("sharpness"), QString::number(m_sharpnessRow->value(), 'f', SHARPNESS_DECIMALS));
    ini.setValue(QStringLiteral("object_motion"), onOff(m_motionRow->isOn()));
    ini.setValue(QStringLiteral("anti_aliasing"), m_antiAliasingRow->value());
    ini.setValue(QStringLiteral("texture_quality"), m_textureRow->value());
    ini.setValue(QStringLiteral("model_lod"), m_lodRow->value());
    ini.setValue(QStringLiteral("vbao"), onOff(m_vbaoRow->isOn()));
    for (auto it = m_effectRows.cbegin(); it != m_effectRows.cend(); ++it)
    {
        ini.setValue(it.key(), onOff(it.value()->isOn()));
    }
}

QString GraphicsPage::frameGeneration() const
{
    return m_frameGenerationRow->value();
}

void GraphicsPage::setGpus(const QList<Capabilities::Gpu>& gpus)
{
    m_gpus = gpus;
    updateFrameGenerations();
}

void GraphicsPage::updateFrameGenerations()
{
    const QString current = m_frameGenerationRow->value();
    const bool dlss = GraphicsSettings::offersDlssFrameGeneration(m_gpus, current);
    QList<ChoiceCombo::Item> items;
    for (const Choices::Choice& choice : Choices::frameGenerations())
    {
        const QString value = QString::fromLatin1(choice.value);
        if (GraphicsSettings::isDlssFrameGeneration(value) && dlss == false) continue;
        items.append({ Choices::label(choice), value, Choices::note(choice) });
    }
    {
        const QSignalBlocker blocker(m_frameGenerationRow->combo());
        m_frameGenerationRow->combo()->setItems(items);
        m_frameGenerationRow->combo()->setValue(current);
    }
    updateFrameGenerationHint();
}

void GraphicsPage::updateFrameGenerationHint()
{
    const QString description = tr("Frames between the game's own. DLSS needs the DLSS bridge, NVIDIA's "
                                   "libnvidia-ngx-dlssg.so (chosen below, like the DLSS library) and a GeForce RTX 40 or "
                                   "newer; 3x and 4x: RTX 50");
    QString hint;
    if (GraphicsSettings::isDlssFrameGeneration(m_frameGenerationRow->value()))
    {
        const QFileInfo library = UpscalerAssets::ngxInUse(m_userDir + QStringLiteral("/dlss"), PortPaths::binariesDir(),
                                                           UpscalerAssets::NGX_FG_PREFIX);
        if (library.fileName().isEmpty())
        {
            hint = tr("No libnvidia-ngx-dlssg.so: choose it below (DLSS SDK 310.7 or newer)");
        }
    }
    m_frameGenerationRow->setBaseSubtitle(describe(description, hint));
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
    updateFrameGenerationHint();
}

void GraphicsPage::setGameRunning(bool running)
{
    m_gameRunning = running;
    updateFsr411Row();
}

void GraphicsPage::updateUpscalerStatus()
{
    const QString value = m_upscalerRow->value();
    const QString description = tr("Draws the scene smaller and reconstructs it at the output resolution");
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
        const QFileInfo library = UpscalerAssets::ngxInUse(m_userDir + QStringLiteral("/dlss"), binaries);
        const bool found = QFileInfo(binaries + QStringLiteral("/libbbport_dlss.so")).isFile()
            && library.fileName().isEmpty() == false;
        // The generation depends on the preset: NVIDIA's defaults differ per quality mode.
        const QString version = UpscalerAssets::ngxVersion(library);
        const QString generation = UpscalerAssets::dlssGeneration(version, m_presetRow->value().toInt());
        const QString name = generation.isEmpty() ? QStringLiteral("DLSS") : QStringLiteral("DLSS %1").arg(generation);
        hint = found ? tr("%1 with this preset (NVIDIA's library %2); needs an NVIDIA RTX GPU").arg(name, version)
                     : tr("DLSS not found: choose NVIDIA's library below (a build with the DLSS bridge is needed too). "
                          "The game will use FSR 3.1");
    }
    m_presetRow->setEnabled(value != QStringLiteral("taa") && value != QStringLiteral("off"));
    // Sharpening works without an upscaler too (RCAS on the finished frame).
    m_upscalerRow->setBaseSubtitle(describe(description, hint));
}

void GraphicsPage::updateFsr411Row(const QString& progress)
{
    const bool building = isBuilding();
    m_fsr411Button->setText(building ? tr("Cancel") : tr("Choose DLL..."));
    m_fsr411Button->setEnabled(building || m_gameRunning == false);
    m_fsr412Button->setText(building ? tr("Cancel") : tr("Choose a file..."));
    m_fsr412Button->setEnabled(building || m_gameRunning == false);
    // The progress goes to the row of the build that runs.
    SettingRow* progressRow = m_buildKind == BuildKind::DriverModels ? m_fsr412Row : m_fsr411Row;
    if (progress.isEmpty() == false)
    {
        progressRow->setSubtitle(progress);
    }
    else if (building == false)
    {
        m_fsr411Row->setSubtitle(tr("Build FSR 4.1.1 from amd_fidelityfx_upscaler_dx12.dll 4.1.x (OptiScaler: the "
                                    "FSR4_LATEST folder, or from a game with FSR 4.1). Needs GE-Proton 10+, Proton "
                                    "Experimental or Proton-CachyOS; 2-5 minutes (RDNA4: twice as long, with the FP8 "
                                    "variant)"));
        m_fsr412Row->setSubtitle(tr("The INT8 RDNA2 and INT8 Lite models: the AMD Adrenalin 26.10.1+ installer (.exe, "
                                    "unpacked with 7z) or the amdxcffx64.dll from it. The FSR 4.1.x DLL is needed too (as "
                                    "for FSR 4.1.1); about 4 minutes. The model is chosen in the game's menu"));
    }
}

void GraphicsPage::updateDlssRow()
{
    const QString ownDirectory = m_userDir + QStringLiteral("/dlss");
    const QString binaries = PortPaths::binariesDir();
    const bool own = UpscalerAssets::ngxLibraries(ownDirectory).isEmpty() == false;
    const bool ownFrames = UpscalerAssets::ngxLibraries(ownDirectory, UpscalerAssets::NGX_FG_PREFIX).isEmpty() == false;
    const QFileInfo library = UpscalerAssets::ngxInUse(ownDirectory, binaries);
    const QFileInfo frames = UpscalerAssets::ngxInUse(ownDirectory, binaries, UpscalerAssets::NGX_FG_PREFIX);
    const bool bridge = QFileInfo(binaries + QStringLiteral("/libbbport_dlss.so")).isFile();
    QString text;
    if (own)
    {
        text = tr("Your own: %1").arg(UpscalerAssets::ngxVersion(library));
    }
    else if (library.fileName().isEmpty() == false)
    {
        text = tr("From this build: %1").arg(UpscalerAssets::ngxVersion(library));
    }
    else
    {
        text = tr("None: choose libnvidia-ngx-dlss.so.* from NVIDIA's DLSS SDK (%1)").arg(UpscalerAssets::DLSS_SDK_URL);
    }
    // The Frame Generation library (the same SDK folder): the game looks in both places.
    if (ownFrames)
    {
        text += u'\n' + tr("Frame generation: your own %1").arg(UpscalerAssets::ngxVersion(frames));
    }
    else if (frames.fileName().isEmpty() == false)
    {
        text += u'\n' + tr("Frame generation: from this build, %1").arg(UpscalerAssets::ngxVersion(frames));
    }
    else
    {
        text += u'\n' + tr("Frame generation: no libnvidia-ngx-dlssg.so.* (choose it the same way; DLSS SDK 310.7 or newer)");
    }
    if (bridge == false)
    {
        text += u'\n' + tr("This build has no DLSS bridge (libbbport_dlss.so): DLSS will not work");
    }
    m_dlssRow->setSubtitle(text);
    m_dlssReset->setEnabled(own || ownFrames);
    updateUpscalerStatus();
    updateFrameGenerationHint();
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
    m_fsr411Dll = upscaler;
    emit changed();
    inspectDll(upscaler, {});
}

void GraphicsPage::chooseDriver()
{
    const QString start = QFileInfo(m_dllDir).isDir() ? m_dllDir : QDir::homePath();
    const QString driver = QFileDialog::getOpenFileName(this, tr("AMD's driver: the Adrenalin installer (.exe) or amdxcffx64.dll"),
                                                        start, tr("AMD driver (*.exe *.EXE *.dll *.DLL)"));
    if (driver.isEmpty()) return;
    // The driver's FSR 4 module answers through AMD's upscaler DLL: the one FSR 4.1.1 was built
    // from, else one chosen now.
    QString upscaler = m_fsr411Dll;
    if (upscaler.isEmpty() || QFileInfo(upscaler).isFile() == false)
    {
        emit toast(tr("The FSR 4.1.x DLL is needed too: choose amd_fidelityfx_upscaler_dx12.dll"));
        upscaler = chooseDll(tr("AMD upscaler DLL (amd_fidelityfx_upscaler_dx12.dll)"));
        if (upscaler.isEmpty()) return;
        m_dllDir = QFileInfo(upscaler).absolutePath();
        m_fsr411Dll = upscaler;
        emit changed();
    }
    startBuild(BuildKind::DriverModels, driver, upscaler);
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
        startBuild(BuildKind::Fsr411, upscaler, details.value(QStringLiteral("loader")).toString());
    }, DLL_CHECK_TIMEOUT_MS);
}

void GraphicsPage::startBuild(BuildKind kind, const QString& first, const QString& second)
{
    if (m_gameRunning) return;
    // FSR 4.1.1: the upscaler DLL and its loader; FSR 4.1.2: the driver and the upscaler DLL.
    const Fsr411Build::Command command = kind == BuildKind::DriverModels ? Fsr411Build::driverCommand(first, second)
                                                                         : Fsr411Build::command(first, second);
    m_buildCanceled = false;
    m_buildKind = kind;
    if (m_build->start(command.program, command.arguments, command.env, command.workingDirectory) == false)
    {
        emit toast(tr("Could not start: %1").arg(m_build->errorString()));
        return;
    }
    emit logLine(QString());
    emit logLine(kind == BuildKind::DriverModels ? tr("--- building FSR 4.1.2's models from %1 ---").arg(first)
                                                 : tr("--- building FSR 4.1.1 from %1 ---").arg(first));
    updateFsr411Row(tr("Preparing..."));
    emit buildRunningChanged(true);
}

void GraphicsPage::buildFinished(int exitCode)
{
    const bool canceled = m_buildCanceled;
    const bool driverModels = m_buildKind == BuildKind::DriverModels;
    m_buildCanceled = false;
    updateFsr411Row();
    emit buildRunningChanged(false);
    emit logLine(tr("--- FSR 4.1 build finished (exit code %1) ---").arg(exitCode));
    if (exitCode == EXIT_SUCCESS)
    {
        m_upscalerRow->combo()->setValue(FSR411);
        updateUpscalerStatus();
        emit changed();
        emit toast(driverModels ? tr("FSR 4.1.2's models built and FSR 4.1 selected: choose the model in the game's menu")
                                : tr("FSR 4.1.1 built and selected"));
        return;
    }
    if (canceled)
    {
        emit toast(tr("FSR 4.1 build canceled"));
        return;
    }
    QMessageBox::warning(this, driverModels ? tr("FSR 4.1.2's models were not built") : tr("FSR 4.1.1 was not built"),
                         Fsr411Build::errorMessage(exitCode));
}

void GraphicsPage::chooseDlssLibrary()
{
    const QString path = QFileDialog::getOpenFileName(this, tr("NVIDIA's DLSS libraries (libnvidia-ngx-dlss.so.*, libnvidia-ngx-dlssg.so.*)"),
                                                      QDir::homePath(),
                                                      tr("libnvidia-ngx-dlss*.so.*, nvngx_dlss*.dll (libnvidia-ngx-dlss.so* "
                                                         "libnvidia-ngx-dlssg.so* *.dll *.DLL)"));
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
    const QString prefix = UpscalerAssets::ngxPrefixOf(file.fileName());
    if (prefix.isEmpty() || UpscalerAssets::isElf(path) == false)
    {
        QMessageBox::warning(this, tr("This is not the DLSS library"),
                             tr("libnvidia-ngx-dlss.so.<version> (DLSS) or libnvidia-ngx-dlssg.so.<version> (frame "
                                "generation) from NVIDIA's DLSS SDK is needed: %1").arg(UpscalerAssets::DLSS_SDK_URL));
        return;
    }
    const QString target = m_userDir + QStringLiteral("/dlss");
    QDir().mkpath(target);
    // One library of each kind.
    for (const QFileInfo& old : UpscalerAssets::ngxLibraries(target, prefix))
    {
        QFile::remove(old.absoluteFilePath());
    }
    if (QFile::copy(path, target + u'/' + file.fileName()) == false)
    {
        QMessageBox::warning(this, tr("Could not copy the library"), tr("Could not copy %1 to %2").arg(path, target));
        return;
    }
    emit toast(prefix == UpscalerAssets::NGX_FG_PREFIX ? tr("DLSS Frame Generation %1: ready").arg(UpscalerAssets::ngxVersion(file))
                                                       : tr("DLSS %1: ready").arg(UpscalerAssets::ngxVersion(file)));
    updateDlssRow();
}

void GraphicsPage::removeDlssLibrary()
{
    const QString directory = m_userDir + QStringLiteral("/dlss");
    for (const QString& prefix : { UpscalerAssets::NGX_PREFIX, UpscalerAssets::NGX_FG_PREFIX })
    {
        for (const QFileInfo& old : UpscalerAssets::ngxLibraries(directory, prefix))
        {
            QFile::remove(old.absoluteFilePath());
        }
    }
    updateDlssRow();
}
