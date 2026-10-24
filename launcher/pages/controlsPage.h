#pragma once

#include <QHash>
#include <QPointer>
#include <QString>
#include <optional>

#include "launcherPage.h"

class ChoiceRow;
class SettingRow;
class SpinRow;
class SwitchRow;

// ControlsPage - the controller the game uses, the keyboard, mouse and gamepad bindings
// (bbport.ini key.<input>= / pad.<input>=) and mouse look.
class ControlsPage : public LauncherPage
{
    Q_OBJECT

public:
    explicit ControlsPage(const LauncherSettings& settings, const PortIni& ini, QWidget* parent = nullptr);

    void store(LauncherSettings& settings, PortIni& ini) const override;

private:
    struct Binding
    {
        SettingRow* row = nullptr;
        QString defaultValue;
        // The saved binding; no value: the default.
        std::optional<QString> value;
    };

    void fillGamepads();
    void showGamepad();
    void showBinding(const QString& key);
    void assign(const QString& kind, const QString& key);
    void setBinding(const QString& key, const std::optional<QString>& value);

    QString m_savedGamepad;
    QString m_savedGamepadName;
    QHash<QString, QString> m_gamepadNames;
    // "key.<input>" / "pad.<input>".
    QHash<QString, Binding> m_bindings;
    QPointer<QObject> m_reading;

    ChoiceRow* m_gamepadRow = nullptr;
    SwitchRow* m_mouseLookRow = nullptr;
    SpinRow* m_sensitivityRow = nullptr;
    SwitchRow* m_invertRow = nullptr;
};
