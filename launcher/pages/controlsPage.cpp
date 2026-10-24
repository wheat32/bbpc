#include "controlsPage.h"

#include <QSignalBlocker>
#include <QToolButton>

#include "core/capabilities.h"
#include "core/choices.h"
#include "process/portQueries.h"
#include "process/processRunner.h"
#include "widgets/choiceCombo.h"
#include "widgets/icons.h"
#include "widgets/settingRows.h"

namespace
{
constexpr int GAMEPAD_QUERY_TIMEOUT_MS = 5000;
constexpr double SENSITIVITY_MIN = 0.1;
constexpr double SENSITIVITY_MAX = 10.0;
constexpr double SENSITIVITY_STEP = 0.05;
constexpr int SENSITIVITY_DECIMALS = 2;
const QString KEY_KIND = QStringLiteral("key");
const QString PAD_KIND = QStringLiteral("pad");
} // namespace

ControlsPage::ControlsPage(const LauncherSettings& settings, const PortIni& ini, QWidget* parent)
    : LauncherPage(parent), m_savedGamepad(settings.gamepad), m_savedGamepadName(settings.gamepadName)
{
    // The first gamepad SDL found was taken, wheels and other controllers too: one can be chosen.
    SettingsGroup* controller = new SettingsGroup(tr("Controller"));
    m_gamepadRow = new ChoiceRow(tr("Controller"), {});
    QToolButton* refresh = Icons::button(Icons::Kind::Refresh, tr("Refresh the list"), m_gamepadRow);
    m_gamepadRow->addSuffix(refresh);
    connect(refresh, &QToolButton::clicked, this, &ControlsPage::fillGamepads);
    connect(m_gamepadRow, &ChoiceRow::valueChanged, this, [this](const QString& value)
    {
        m_savedGamepad = value;
        m_savedGamepadName = m_gamepadNames.value(value);
        showGamepad();
        emit changed();
    });
    controller->addRow(m_gamepadRow);

    // Mouse look (runtime_pad.c): a click in the game takes the mouse, F1 lets it go; the mouse
    // turns the game's camera itself.
    m_mouseLookRow = new SwitchRow(tr("Mouse look"), tr("A click in the game window takes the mouse, F1 releases it"),
                                   ini.value(QStringLiteral("mouse_look"), QStringLiteral("1")) == QStringLiteral("1"));
    controller->addRow(m_mouseLookRow);
    m_sensitivityRow = new SpinRow(tr("Mouse sensitivity"), {}, SENSITIVITY_MIN, SENSITIVITY_MAX, SENSITIVITY_STEP,
                                   SENSITIVITY_DECIMALS, ini.value(QStringLiteral("mouse_sensitivity"), QStringLiteral("1.0")).toDouble());
    connect(m_sensitivityRow, &SpinRow::valueChanged, this, &ControlsPage::changed);
    controller->addRow(m_sensitivityRow);
    m_invertRow = new SwitchRow(tr("Invert mouse Y axis"), {}, ini.value(QStringLiteral("mouse_invert_y")) == QStringLiteral("1"));
    connect(m_invertRow, &SwitchRow::toggled, this, &ControlsPage::changed);
    controller->addRow(m_invertRow);
    const auto mouseRowsEnabled = [this]()
    {
        m_sensitivityRow->setEnabled(m_mouseLookRow->isOn());
        m_invertRow->setEnabled(m_mouseLookRow->isOn());
    };
    connect(m_mouseLookRow, &SwitchRow::toggled, this, [this, mouseRowsEnabled]()
    {
        mouseRowsEnabled();
        emit changed();
    });
    mouseRowsEnabled();
    addGroup(controller);

    // Bindings: "Assign" waits for a key or a button (bb-gpu-capabilities --read-input).
    struct Section
    {
        QString kind;
        QString title;
        Icons::Kind icon;
    };
    const QList<Section> sections = {
        { KEY_KIND, tr("Keyboard and mouse"), Icons::Kind::Keyboard },
        { PAD_KIND, tr("Gamepad"), Icons::Kind::Gamepad },
    };
    for (const Section& section : sections)
    {
        SettingsGroup* group = new SettingsGroup(section.title, tr("Button assignments; applied when the game starts"));
        group->makeCollapsible(false);
        for (const Choices::Control& control : Choices::controls())
        {
            const char* defaultValue = section.kind == KEY_KIND ? control.keyDefault : control.padDefault;
            if (defaultValue == nullptr) continue;
            const QString key = section.kind + u'.' + QString::fromLatin1(control.name);
            SettingRow* row = new SettingRow(Choices::label(control));
            QToolButton* assignButton = Icons::button(section.icon, tr("Assign"), row);
            QToolButton* resetButton = Icons::button(Icons::Kind::Undo, tr("Reset"), row);
            row->addSuffix(assignButton);
            row->addSuffix(resetButton);
            const QString kind = section.kind;
            connect(assignButton, &QToolButton::clicked, this, [this, kind, key]()
            {
                assign(kind, key);
            });
            connect(resetButton, &QToolButton::clicked, this, [this, key]()
            {
                setBinding(key, std::nullopt);
            });
            Binding binding;
            binding.row = row;
            binding.defaultValue = QString::fromLatin1(defaultValue);
            if (ini.contains(key))
            {
                binding.value = ini.value(key);
            }
            m_bindings.insert(key, binding);
            showBinding(key);
            group->addRow(row);
        }
        addGroup(group);
    }
    finish();

    m_gamepadRow->combo()->setItems({ { tr("Auto"), QString(), tr("First connected") } });
    showGamepad();
    fillGamepads();
}

void ControlsPage::store(LauncherSettings& settings, PortIni& ini) const
{
    settings.gamepad = m_savedGamepad;
    settings.gamepadName = m_savedGamepadName;
    ini.setValue(QStringLiteral("mouse_look"), m_mouseLookRow->isOn() ? QStringLiteral("1") : QStringLiteral("0"));
    ini.setValue(QStringLiteral("mouse_sensitivity"), QString::number(m_sensitivityRow->value(), 'f', SENSITIVITY_DECIMALS));
    ini.setValue(QStringLiteral("mouse_invert_y"), m_invertRow->isOn() ? QStringLiteral("1") : QStringLiteral("0"));
    for (auto it = m_bindings.cbegin(); it != m_bindings.cend(); ++it)
    {
        if (it.value().value.has_value())
        {
            ini.setValue(it.key(), it.value().value.value());
        }
        else
        {
            ini.remove(it.key());
        }
    }
}

void ControlsPage::fillGamepads()
{
    PortQueries::capabilities({ QStringLiteral("--gamepads") }, this, [this](const QString& output)
    {
        const QString saved = m_savedGamepad;
        const QString savedName = m_savedGamepadName;
        QList<ChoiceCombo::Item> items = { { tr("Auto"), QString(), tr("First connected") } };
        m_gamepadNames.clear();
        for (const Capabilities::Gamepad& gamepad : Capabilities::parseGamepads(output))
        {
            items.append({ gamepad.name, gamepad.guid, gamepad.name });
            m_gamepadNames.insert(gamepad.guid, gamepad.name);
        }
        if (saved.isEmpty() == false && m_gamepadNames.contains(saved) == false)
        {
            const QString name = savedName.isEmpty() ? saved : savedName;
            items.append({ tr("%1 (not connected)").arg(name), saved, name });
            m_gamepadNames.insert(saved, savedName);
        }
        {
            const QSignalBlocker blocker(m_gamepadRow->combo());
            m_gamepadRow->combo()->setItems(items);
            m_gamepadRow->combo()->setValue(saved);
        }
        m_savedGamepad = saved;
        m_savedGamepadName = savedName.isEmpty() ? m_gamepadNames.value(saved) : savedName;
        showGamepad();
    }, GAMEPAD_QUERY_TIMEOUT_MS);
}

void ControlsPage::showGamepad()
{
    // Controller names are long: the selected one is shown in full under the title.
    QStringList lines;
    if (m_gamepadRow->combo()->note().isEmpty() == false)
    {
        lines.append(m_gamepadRow->combo()->note());
    }
    lines.append(tr("The chosen one is used as soon as it connects"));
    m_gamepadRow->setBaseSubtitle(QString());
    m_gamepadRow->setSubtitle(lines.join(u'\n'));
}

void ControlsPage::showBinding(const QString& key)
{
    const Binding& binding = m_bindings[key];
    if (binding.value.has_value() == false)
    {
        binding.row->setSubtitle(binding.defaultValue.isEmpty() ? tr("not assigned") : tr("%1 (default)").arg(binding.defaultValue));
        return;
    }
    binding.row->setSubtitle(binding.value->isEmpty() ? tr("not assigned") : binding.value.value());
}

void ControlsPage::setBinding(const QString& key, const std::optional<QString>& value)
{
    m_bindings[key].value = value;
    showBinding(key);
    emit changed();
}

void ControlsPage::assign(const QString& kind, const QString& key)
{
    // One assignment at a time: the tool waits for a single key or button.
    if (m_reading != nullptr)
    {
        ProcessRunner::cancel(m_reading);
    }
    m_bindings[key].row->setSubtitle(tr("Press a key or a button... (Esc cancels)"));
    m_reading = PortQueries::capabilities({ QStringLiteral("--read-input"), kind }, this, [this, kind, key](const QString& output)
    {
        const std::optional<QString> input = Capabilities::parseInput(output, kind);
        if (input.has_value())
        {
            setBinding(key, input);
            return;
        }
        showBinding(key);
    }, ProcessRunner::NO_TIMEOUT);
    if (m_reading == nullptr)
    {
        showBinding(key);
    }
}
