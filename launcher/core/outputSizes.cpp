#include "outputSizes.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace
{
constexpr int MIN_WIDTH = 640;
constexpr int MAX_WIDTH = 7680;
constexpr int MIN_HEIGHT = 360;
constexpr int MAX_HEIGHT = 4320;
constexpr qsizetype WIDTH_PART = 0;
constexpr qsizetype HEIGHT_PART = 1;
constexpr qsizetype SIZE_PARTS = 2;
const QString FALLBACK_ASPECT = QStringLiteral("16:9");

struct Aspect
{
    QString name;
    double ratio;
    QList<QSize> sizes;
};

const QList<Aspect>& table()
{
    static const QList<Aspect> list = {
        { QStringLiteral("4:3"), 4.0 / 3.0,
          { { 640, 480 }, { 800, 600 }, { 1024, 768 }, { 1280, 960 }, { 1440, 1080 }, { 1600, 1200 },
            { 2048, 1536 }, { 2880, 2160 } } },
        { QStringLiteral("16:10"), 16.0 / 10.0,
          { { 640, 400 }, { 960, 600 }, { 1280, 800 }, { 1440, 900 }, { 1680, 1050 }, { 1920, 1200 },
            { 2560, 1600 }, { 3840, 2400 } } },
        { QStringLiteral("16:9"), 16.0 / 9.0,
          { { 640, 360 }, { 854, 480 }, { 960, 540 }, { 1280, 720 }, { 1600, 900 }, { 1920, 1080 },
            { 2560, 1440 }, { 3200, 1800 }, { 3840, 2160 } } },
        { QStringLiteral("21:9"), 64.0 / 27.0,
          { { 860, 360 }, { 1280, 540 }, { 1720, 720 }, { 2560, 1080 }, { 3440, 1440 }, { 3840, 1600 },
            { 5120, 2160 } } },
        { QStringLiteral("32:9"), 32.0 / 9.0,
          { { 1280, 360 }, { 1920, 540 }, { 2560, 720 }, { 3840, 1080 }, { 5120, 1440 }, { 7680, 2160 } } },
    };
    return list;
}

const Aspect* find(const QString& name)
{
    for (const Aspect& aspect : table())
    {
        if (aspect.name == name)
        {
            return &aspect;
        }
    }
    return nullptr;
}

bool fitsOutputRange(const QSize& size)
{
    return size.width() >= MIN_WIDTH && size.width() <= MAX_WIDTH
        && size.height() >= MIN_HEIGHT && size.height() <= MAX_HEIGHT;
}
} // namespace

const QStringList& OutputSizes::aspects()
{
    static const QStringList list = []()
    {
        QStringList names = { AUTO };
        for (const Aspect& aspect : table())
        {
            names.append(aspect.name);
        }
        return names;
    }();
    return list;
}

QString OutputSizes::resolvedAspect(const QString& aspect, const std::optional<QSize>& monitor)
{
    if (find(aspect) != nullptr)
    {
        return aspect;
    }
    if (monitor.has_value() == false || monitor->height() <= 0)
    {
        return FALLBACK_ASPECT;
    }
    const double ratio = static_cast<double>(monitor->width()) / monitor->height();
    QString best;
    double bestDistance = std::numeric_limits<double>::infinity();
    for (const Aspect& candidate : table())
    {
        const double distance = std::abs(std::log(ratio / candidate.ratio));
        if (distance < bestDistance)
        {
            bestDistance = distance;
            best = candidate.name;
        }
    }
    return best;
}

QList<QSize> OutputSizes::sizes(const QString& aspect, const std::optional<QSize>& monitor)
{
    QList<QSize> list = find(resolvedAspect(aspect, monitor))->sizes;
    if (aspect == AUTO && monitor.has_value() && list.contains(monitor.value()) == false
        && fitsOutputRange(monitor.value()))
    {
        list.append(monitor.value());
        std::stable_sort(list.begin(), list.end(), [](const QSize& a, const QSize& b)
        {
            return a.height() != b.height() ? a.height() < b.height() : a.width() < b.width();
        });
    }
    return list;
}

QSize OutputSizes::nearest(const QList<QSize>& sizes, const QSize& size)
{
    if (sizes.contains(size) || sizes.isEmpty())
    {
        return size;
    }
    QSize best = sizes.first();
    for (const QSize& candidate : sizes)
    {
        if (std::abs(candidate.height() - size.height()) < std::abs(best.height() - size.height()))
        {
            best = candidate;
        }
    }
    return best;
}

std::optional<QSize> OutputSizes::parse(const QString& text)
{
    const QStringList parts = text.trimmed().toLower().split(u'x');
    if (parts.size() != SIZE_PARTS)
    {
        return std::nullopt;
    }
    bool widthOk = false;
    bool heightOk = false;
    const int width = parts.at(WIDTH_PART).toInt(&widthOk);
    const int height = parts.at(HEIGHT_PART).toInt(&heightOk);
    if (widthOk == false || heightOk == false || width <= 0 || height <= 0)
    {
        return std::nullopt;
    }
    return QSize(width, height);
}

QString OutputSizes::format(const QSize& size)
{
    return QString::number(size.width()) + u'x' + QString::number(size.height());
}
