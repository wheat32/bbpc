#include "capabilities.h"

#include <QStringList>

#include "outputSizes.h"

namespace
{
// The tab-separated fields of a --gpus line.
constexpr int GPU_INDEX_FIELD = 0;
constexpr int GPU_VENDOR_FIELD = 1;
constexpr int GPU_KIND_FIELD = 2;
constexpr int GPU_NAME_FIELD = 3;
constexpr int GPU_BEST_FIELD = 4;
constexpr int GPU_FIELDS = 5;
// The tab-separated fields of a --displays line.
constexpr int DISPLAY_NAME_FIELD = 0;
constexpr int DISPLAY_SIZE_FIELD = 1;
constexpr int DISPLAY_PRIMARY_FIELD = 2;
constexpr int DISPLAY_FIELDS = 3;

QStringList lines(const QString& output)
{
    return output.split(u'\n', Qt::SkipEmptyParts);
}
} // namespace

QList<Capabilities::Gpu> Capabilities::parseGpus(const QString& output)
{
    QList<Gpu> gpus;
    for (const QString& line : lines(output))
    {
        const QStringList fields = line.split(u'\t');
        if (fields.size() != GPU_FIELDS) continue;
        bool ok = false;
        const int index = fields.at(GPU_INDEX_FIELD).toInt(&ok);
        if (ok == false) continue;
        gpus.append({ index, fields.at(GPU_VENDOR_FIELD), fields.at(GPU_KIND_FIELD), fields.at(GPU_NAME_FIELD),
                      fields.at(GPU_BEST_FIELD).trimmed() == QStringLiteral("1") });
    }
    return gpus;
}

QList<Capabilities::Display> Capabilities::parseDisplays(const QString& output)
{
    QList<Display> displays;
    QStringList names;
    for (const QString& line : lines(output))
    {
        const QStringList fields = line.split(u'\t');
        if (fields.size() != DISPLAY_FIELDS) continue;
        Display display;
        display.name = fields.at(DISPLAY_NAME_FIELD);
        display.number = static_cast<int>(displays.size()) + 1;
        display.size = OutputSizes::parse(fields.at(DISPLAY_SIZE_FIELD));
        display.primary = fields.at(DISPLAY_PRIMARY_FIELD).trimmed() == QStringLiteral("1");
        displays.append(display);
        names.append(display.name);
    }
    for (Display& display : displays)
    {
        display.value = names.count(display.name) == 1 ? display.name : QString::number(display.number);
    }
    return displays;
}

QList<Capabilities::Gamepad> Capabilities::parseGamepads(const QString& output)
{
    QList<Gamepad> gamepads;
    for (const QString& line : lines(output))
    {
        const qsizetype tab = line.indexOf(u'\t');
        if (tab < 0) continue;
        gamepads.append({ line.left(tab), line.mid(tab + 1) });
    }
    return gamepads;
}

std::optional<QString> Capabilities::parseInput(const QString& output, const QString& kind)
{
    const QString text = output.trimmed();
    const qsizetype space = text.indexOf(u' ');
    if (space < 0 || text.left(space) != kind)
    {
        return std::nullopt;
    }
    return text.mid(space + 1);
}
