#pragma once

#include <QList>
#include <QSize>
#include <QString>
#include <optional>

// Capabilities - what bb-gpu-capabilities (tools/gpu_capabilities.c) reports about this machine,
// parsed from its tab-separated output.
namespace Capabilities
{
    // --gpus: "<index>\t<vendor ID>\t<type>\t<name>\t<the port's own choice: 0/1>", in Vulkan's
    // order (BB_GPU_ID is the index).
    struct Gpu
    {
        int index = 0;
        QString vendor;
        QString kind;
        QString name;
        bool best = false;
    };
    QList<Gpu> parseGpus(const QString& output);

    // --displays: "<name>\t<W>x<H>\t<primary: 0/1>". value is what BB_DISPLAY takes: the name, or
    // the monitor's number when two monitors share a name.
    struct Display
    {
        QString value;
        QString name;
        int number = 0;
        std::optional<QSize> size;
        bool primary = false;
    };
    QList<Display> parseDisplays(const QString& output);

    // --gamepads: "<GUID>\t<name>".
    struct Gamepad
    {
        QString guid;
        QString name;
    };
    QList<Gamepad> parseGamepads(const QString& output);

    // --read-input key|pad: "key <name>" or "pad <name>" once one is pressed; nothing on Escape.
    std::optional<QString> parseInput(const QString& output, const QString& kind);
}
