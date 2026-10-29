#pragma once

#include <QList>
#include <QString>

#include "capabilities.h"

class PortIni;

// GraphicsSettings - how the launcher's graphics choices map to bbport.ini, where one choice is
// two keys or follows another (gpu/shim/bbport_settings.cpp reads them).
namespace GraphicsSettings
{
    inline const QString FRAME_GENERATION_OFF = QStringLiteral("off");
    inline const QString FRAME_GENERATION_FSR3 = QStringLiteral("fsr3");
    inline const QString DLSS_PREFIX = QStringLiteral("dlss");
    inline const QString LOW_LATENCY_ON = QStringLiteral("on");
    inline const QString LOW_LATENCY_OFF = QStringLiteral("off");

    // DLSS Frame Generation's multipliers (frames shown per game frame): 2 (RTX 40), up to 4 (RTX 50).
    inline constexpr int MIN_MULTIPLIER = 2;
    inline constexpr int MAX_MULTIPLIER = 4;

    // The launcher's frame generation choice (Choices::frameGenerations: off, fsr3, dlss2..dlss4)
    // for bbport.ini's frame_generation and frame_generation_multiplier. "1" of older settings is
    // FSR 3.1, as the game reads it; anything else is off.
    QString frameGenerationChoice(const QString& frameGeneration, const QString& multiplier);

    // bbport.ini's frame_generation and frame_generation_multiplier for a choice; the multiplier
    // only DLSS sets is kept as it is for the others.
    struct FrameGenerationKeys
    {
        QString frameGeneration;
        QString multiplier;
    };
    FrameGenerationKeys frameGenerationKeys(const QString& choice, const QString& currentMultiplier);

    [[nodiscard]] bool isDlssFrameGeneration(const QString& choice);

    // DLSS Frame Generation is offered where an NVIDIA GPU is connected, while the GPUs are not
    // known, and while it is the saved choice.
    [[nodiscard]] bool offersDlssFrameGeneration(const QList<Capabilities::Gpu>& gpus, const QString& currentChoice);

    // bbport.ini low_latency as a choice (on, boost, off; the game's numbers 0-2 too), "on" for
    // anything else. With DLSS Frame Generation it is at least on, as the game runs it.
    QString lowLatency(const QString& value, const QString& frameGenerationChoice);

    // bbport.ini anti_aliasing; settings from before it: the game's own AA switched off stays off.
    QString antiAliasing(const PortIni& ini);
}
