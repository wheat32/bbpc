#include "graphicsSettings.h"

#include <algorithm>

#include "choices.h"
#include "portIni.h"

namespace
{
const QString DLSS = QStringLiteral("dlss");
const QString ANTI_ALIASING_KEY = QStringLiteral("anti_aliasing");
const QString GAME_AA_KEY = QStringLiteral("effect_game_aa");
const QString ANTI_ALIASING_AUTO = QStringLiteral("auto");
const QString OFF = QStringLiteral("off");
// The vendor ID bb-gpu-capabilities prints for NVIDIA ("0x10de").
const QString NVIDIA_VENDOR = QStringLiteral("10de");
// The game's numbers for low_latency (BbSettings::LowLatencyMode), by their names.
const QList<QPair<QString, QString>> LOW_LATENCY_NUMBERS = {
    { QStringLiteral("0"), QStringLiteral("off") },
    { QStringLiteral("1"), QStringLiteral("on") },
    { QStringLiteral("2"), QStringLiteral("boost") },
};

bool listed(const QList<Choices::Choice>& choices, const QString& value)
{
    for (const Choices::Choice& choice : choices)
    {
        if (QString::fromLatin1(choice.value) == value) return true;
    }
    return false;
}
} // namespace

QString GraphicsSettings::frameGenerationChoice(const QString& frameGeneration, const QString& multiplier)
{
    const QString value = frameGeneration.trimmed();
    if (value == DLSS)
    {
        bool ok = false;
        const int number = multiplier.trimmed().toInt(&ok);
        const int clamped = ok ? std::clamp(number, MIN_MULTIPLIER, MAX_MULTIPLIER) : MIN_MULTIPLIER;
        return DLSS_PREFIX + QString::number(clamped);
    }
    if (value == FRAME_GENERATION_FSR3)
    {
        return FRAME_GENERATION_FSR3;
    }
    // Earlier versions wrote 1 and 0: the game reads any other number than 0 as FSR 3.1.
    bool numeric = false;
    const int number = value.toInt(&numeric);
    return numeric && number != 0 ? FRAME_GENERATION_FSR3 : FRAME_GENERATION_OFF;
}

GraphicsSettings::FrameGenerationKeys GraphicsSettings::frameGenerationKeys(const QString& choice, const QString& currentMultiplier)
{
    if (isDlssFrameGeneration(choice))
    {
        return { DLSS, choice.mid(DLSS_PREFIX.size()) };
    }
    const QString value = choice == FRAME_GENERATION_FSR3 ? FRAME_GENERATION_FSR3 : FRAME_GENERATION_OFF;
    return { value, currentMultiplier };
}

bool GraphicsSettings::isDlssFrameGeneration(const QString& choice)
{
    return choice.startsWith(DLSS_PREFIX) && choice.size() > DLSS_PREFIX.size();
}

bool GraphicsSettings::offersDlssFrameGeneration(const QList<Capabilities::Gpu>& gpus, const QString& currentChoice)
{
    if (gpus.isEmpty() || isDlssFrameGeneration(currentChoice)) return true;
    for (const Capabilities::Gpu& gpu : gpus)
    {
        if (gpu.vendor.contains(NVIDIA_VENDOR, Qt::CaseInsensitive)) return true;
    }
    return false;
}

QString GraphicsSettings::lowLatency(const QString& value, const QString& frameGenerationChoice)
{
    QString mode = value.trimmed();
    for (const auto& [number, name] : LOW_LATENCY_NUMBERS)
    {
        if (mode == number)
        {
            mode = name;
        }
    }
    if (listed(Choices::lowLatency(), mode) == false)
    {
        mode = LOW_LATENCY_ON;
    }
    // NVIDIA requires Reflex with DLSS Frame Generation: generated frames add latency it takes back.
    if (isDlssFrameGeneration(frameGenerationChoice) && mode == LOW_LATENCY_OFF)
    {
        mode = LOW_LATENCY_ON;
    }
    return mode;
}

QString GraphicsSettings::antiAliasing(const PortIni& ini)
{
    if (ini.inFile(ANTI_ALIASING_KEY))
    {
        const QString value = ini.value(ANTI_ALIASING_KEY).trimmed();
        return listed(Choices::antiAliasing(), value) ? value : ANTI_ALIASING_AUTO;
    }
    return ini.value(GAME_AA_KEY).trimmed() == QStringLiteral("0") ? OFF : ANTI_ALIASING_AUTO;
}
