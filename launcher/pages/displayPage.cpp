#include "displayPage.h"

#include <QSignalBlocker>
#include <QToolButton>

#include "core/capabilities.h"
#include "core/choices.h"
#include "core/outputSizes.h"
#include "process/portQueries.h"
#include "widgets/choiceCombo.h"
#include "widgets/icons.h"
#include "widgets/settingRows.h"

namespace
{
constexpr int DISPLAY_QUERY_TIMEOUT_MS = 5000;
} // namespace

DisplayPage::DisplayPage(const LauncherSettings& settings, const PortIni& ini, QWidget* parent)
    : LauncherPage(parent), m_savedDisplay(settings.display)
{
    SettingsGroup* screen = new SettingsGroup(tr("Display"));

    // The aspect ratio, then its resolutions (rebuilt when it changes, the height kept).
    m_aspectRow = new ChoiceRow(tr("Aspect ratio"), tr("Wider than 16:9 sees more to the sides, taller more above and "
                                                       "below; the UI keeps to the screen edges"));
    screen->addRow(m_aspectRow);
    m_outputRow = new ChoiceRow(tr("Output resolution"),
                                tr("The size of the final image, menus included. The game renders at it natively with "
                                   "Native AA, TAA or the upscaler off; the other presets render the scene smaller and "
                                   "upscale it to this size. Steam Deck: 1280×800 (16:10)"));
    screen->addRow(m_outputRow);
    // "auto" of older settings is off, as unset.
    const QString live = ini.value(QStringLiteral("live_resolution")) == QStringLiteral("1") ? QStringLiteral("1")
                                                                                              : QStringLiteral("0");
    m_liveRow = new ChoiceRow(tr("Live resolution changes"),
                              tr("Change the resolution and preset in game without a restart, but about 10% slower"),
                              Choices::liveResolution(), live);
    connect(m_liveRow, &ChoiceRow::valueChanged, this, &DisplayPage::changed);
    screen->addRow(m_liveRow);

    m_fullscreenRow = new SwitchRow(tr("Fullscreen"), {}, settings.fullscreen);
    connect(m_fullscreenRow, &SwitchRow::toggled, this, &DisplayPage::changed);
    screen->addRow(m_fullscreenRow);

    // The game's window opens on this monitor (fullscreen too), not on the one SDL calls primary.
    m_displayRow = new ChoiceRow(tr("Monitor"), {});
    QToolButton* refreshDisplays = Icons::button(Icons::Kind::Refresh, tr("Refresh the list"), m_displayRow);
    m_displayRow->addSuffix(refreshDisplays);
    connect(refreshDisplays, &QToolButton::clicked, this, &DisplayPage::fillDisplays);
    screen->addRow(m_displayRow);

    m_presentRow = new ChoiceRow(tr("Present mode"), {}, Choices::presentModes(), settings.presentMode);
    connect(m_presentRow, &ChoiceRow::valueChanged, this, &DisplayPage::changed);
    screen->addRow(m_presentRow);
    m_hdrRow = new SwitchRow(tr("Allow HDR"), {}, settings.hdr);
    connect(m_hdrRow, &SwitchRow::toggled, this, &DisplayPage::changed);
    screen->addRow(m_hdrRow);
    m_showFpsRow = new SwitchRow(tr("Show FPS"), tr("The frame rate, over the game"),
                                 ini.value(QStringLiteral("show_fps")) == QStringLiteral("1"));
    connect(m_showFpsRow, &SwitchRow::toggled, this, &DisplayPage::changed);
    screen->addRow(m_showFpsRow);
    addGroup(screen);
    finish();

    const QString aspect = ini.value(QStringLiteral("aspect_ratio"), OutputSizes::AUTO);
    m_displayRow->combo()->setItems({ { tr("Primary"), QString(), {} } });
    fillAspects(OutputSizes::aspects().contains(aspect) ? aspect : OutputSizes::AUTO);
    fillOutputs(OutputSizes::parse(ini.value(QStringLiteral("output_res"))).value_or(OutputSizes::DEFAULT_SIZE));

    connect(m_aspectRow, &ChoiceRow::valueChanged, this, [this]()
    {
        fillOutputs(OutputSizes::parse(outputResolution()).value_or(OutputSizes::DEFAULT_SIZE));
        emit changed();
    });
    connect(m_outputRow, &ChoiceRow::valueChanged, this, [this](const QString& output)
    {
        emit outputResolutionChanged(output);
        emit changed();
    });
    connect(m_displayRow, &ChoiceRow::valueChanged, this, [this](const QString& value)
    {
        m_savedDisplay = value;
        const QSize output = OutputSizes::parse(outputResolution()).value_or(OutputSizes::DEFAULT_SIZE);
        fillAspects(m_aspectRow->value());
        fillOutputs(output);
        emit changed();
    });
    fillDisplays();
}

QString DisplayPage::outputResolution() const
{
    return m_outputRow->value();
}

void DisplayPage::store(LauncherSettings& settings, PortIni& ini) const
{
    settings.fullscreen = m_fullscreenRow->isOn();
    settings.display = m_savedDisplay;
    settings.presentMode = m_presentRow->value();
    settings.hdr = m_hdrRow->isOn();
    ini.setValue(QStringLiteral("aspect_ratio"), m_aspectRow->value());
    ini.setValue(QStringLiteral("output_res"), m_outputRow->value());
    ini.setValue(QStringLiteral("live_resolution"), m_liveRow->value());
    ini.setValue(QStringLiteral("show_fps"), m_showFpsRow->isOn() ? QStringLiteral("1") : QStringLiteral("0"));
}

std::optional<QSize> DisplayPage::monitorSize() const
{
    if (m_displaySizes.contains(m_savedDisplay))
    {
        return m_displaySizes.value(m_savedDisplay);
    }
    if (m_displaySizes.contains(QString()))
    {
        return m_displaySizes.value(QString());
    }
    return std::nullopt;
}

void DisplayPage::fillAspects(const QString& current)
{
    QList<ChoiceCombo::Item> items;
    for (const QString& aspect : OutputSizes::aspects())
    {
        const QString label = aspect == OutputSizes::AUTO
            ? tr("Auto (%1)").arg(OutputSizes::resolvedAspect(OutputSizes::AUTO, monitorSize()))
            : aspect;
        items.append({ label, aspect, {} });
    }
    const QSignalBlocker blocker(m_aspectRow->combo());
    m_aspectRow->combo()->setItems(items);
    m_aspectRow->combo()->setValue(current);
}

void DisplayPage::fillOutputs(const QSize& keep)
{
    const QList<QSize> sizes = OutputSizes::sizes(m_aspectRow->value(), monitorSize());
    const QSize chosen = OutputSizes::nearest(sizes, keep);
    QList<ChoiceCombo::Item> items;
    for (const QSize& size : sizes)
    {
        QString label = QStringLiteral("%1×%2").arg(size.width()).arg(size.height());
        if (size == OutputSizes::STEAM_DECK_SIZE)
        {
            label += QStringLiteral(" (Steam Deck)");
        }
        items.append({ label, OutputSizes::format(size), {} });
    }
    {
        const QSignalBlocker blocker(m_outputRow->combo());
        m_outputRow->combo()->setItems(items);
        m_outputRow->combo()->setValue(OutputSizes::format(chosen));
    }
    emit outputResolutionChanged(outputResolution());
}

void DisplayPage::fillDisplays()
{
    PortQueries::capabilities({ QStringLiteral("--displays") }, this, [this](const QString& output)
    {
        const QList<Capabilities::Display> displays = Capabilities::parseDisplays(output);
        m_displaySizes.clear();
        QList<ChoiceCombo::Item> items = { { tr("Primary"), QString(), {} } };
        QStringList values;
        for (const Capabilities::Display& display : displays)
        {
            if (display.size.has_value())
            {
                m_displaySizes.insert(display.value, display.size.value());
                if (display.primary)
                {
                    m_displaySizes.insert(QString(), display.size.value());
                }
            }
            const QString size = display.size.has_value() ? OutputSizes::format(display.size.value()) : QStringLiteral("?");
            QString label = QStringLiteral("%1: %2 (%3)").arg(display.number).arg(display.name, size);
            if (display.primary)
            {
                label += tr(", primary");
            }
            items.append({ label, display.value, {} });
            values.append(display.value);
        }
        const QString saved = m_savedDisplay;
        if (saved.isEmpty() == false && values.contains(saved) == false)
        {
            items.append({ tr("%1 (not connected)").arg(saved), saved, {} });
        }
        const QSize keep = OutputSizes::parse(outputResolution()).value_or(OutputSizes::DEFAULT_SIZE);
        {
            const QSignalBlocker blocker(m_displayRow->combo());
            m_displayRow->combo()->setItems(items);
            m_displayRow->combo()->setValue(saved);
        }
        m_savedDisplay = saved;
        // Auto's aspect ratio and its resolutions follow the monitor.
        fillAspects(m_aspectRow->value());
        fillOutputs(keep);
    }, DISPLAY_QUERY_TIMEOUT_MS);
}
