#pragma once

#include <QList>
#include <QSize>
#include <QString>
#include <QStringList>
#include <optional>

// OutputSizes - output aspect ratios and their resolutions, the same lists as the in-game menu's
// (gpu/shim/bbport_settings.cpp). The game keeps its 1920x1080 frame; for another aspect ratio its
// camera sees more to the sides (or above and below) and the UI keeps to the screen's edges.
// As an aspect ratio "auto" is the group nearest the monitor's; as the output resolution
// (bbport.ini output_res=auto, the default) it is the monitor's own resolution (run.sh passes it
// as BB_MONITOR_SIZE).
namespace OutputSizes
{
    inline const QString AUTO = QStringLiteral("auto");
    inline const QSize STEAM_DECK_SIZE { 1280, 800 };
    inline const QSize DEFAULT_SIZE { 1920, 1080 };

    // "auto", then the ratios ("4:3", "16:10", "16:9", "21:9", "32:9").
    const QStringList& aspects();

    // The aspect ratio group: auto is the one nearest the monitor's (16:9 when it is unknown).
    QString resolvedAspect(const QString& aspect, const std::optional<QSize>& monitor);

    // The resolutions of an aspect ratio, by height; auto adds the monitor's own size.
    QList<QSize> sizes(const QString& aspect, const std::optional<QSize>& monitor);

    // The size itself when listed, else the one nearest in height (another ratio keeps it).
    QSize nearest(const QList<QSize>& sizes, const QSize& size);

    // "WxH" (bbport.ini's output_res) and back.
    std::optional<QSize> parse(const QString& text);
    QString format(const QSize& size);

    // The size an output_res value comes to: auto is the monitor's (1920x1080 while it is not
    // known), an unreadable value 1920x1080.
    QSize effective(const QString& output, const std::optional<QSize>& monitor);
}
