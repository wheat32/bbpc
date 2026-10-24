#pragma once

#include <QFrame>
#include <QString>

#include "core/choices.h"

class ChoiceCombo;
class QDoubleSpinBox;
class QHBoxLayout;
class QLabel;
class QLineEdit;
class ToggleSwitch;

// SettingRow - one line of a settings group: a title, an optional subtitle under it, and the
// row's controls on the right (prefix widgets go left of the title).
class SettingRow : public QFrame
{
    Q_OBJECT

public:
    explicit SettingRow(const QString& title, const QString& subtitle = {}, QWidget* parent = nullptr);

    void setTitle(const QString& title);
    [[nodiscard]] QString title() const;
    // Hidden while empty.
    void setSubtitle(const QString& subtitle);
    [[nodiscard]] QString subtitle() const;

    void addPrefix(QWidget* widget);
    void addSuffix(QWidget* widget, int stretch = 0);
    void removeWidget(QWidget* widget);

private:
    QHBoxLayout* m_prefix = nullptr;
    QHBoxLayout* m_suffix = nullptr;
    QLabel* m_title = nullptr;
    QLabel* m_subtitle = nullptr;
};

// A row with an on/off switch; a click anywhere on the row flips it.
class SwitchRow : public SettingRow
{
    Q_OBJECT

public:
    SwitchRow(const QString& title, const QString& subtitle, bool on, QWidget* parent = nullptr);

    [[nodiscard]] bool isOn() const;
    void setOn(bool on);

signals:
    void toggled(bool on);

protected:
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    ToggleSwitch* m_switch = nullptr;
};

// A row with a drop-down. The selected choice's note is shown under the base subtitle.
class ChoiceRow : public SettingRow
{
    Q_OBJECT

public:
    ChoiceRow(const QString& title, const QString& subtitle, QWidget* parent = nullptr);
    ChoiceRow(const QString& title, const QString& subtitle, const QList<Choices::Choice>& choices,
              const QString& value, QWidget* parent = nullptr);

    [[nodiscard]] ChoiceCombo* combo() const
    {
        return m_combo;
    }
    [[nodiscard]] QString value() const;
    void setBaseSubtitle(const QString& subtitle);

signals:
    void valueChanged(const QString& value);

private:
    void showNote();

    ChoiceCombo* m_combo = nullptr;
    QString m_baseSubtitle;
};

// A row with a number field.
class SpinRow : public SettingRow
{
    Q_OBJECT

public:
    SpinRow(const QString& title, const QString& subtitle, double minimum, double maximum, double step, int decimals,
            double value, QWidget* parent = nullptr);

    [[nodiscard]] double value() const;
    void setValue(double value);

signals:
    void valueChanged(double value);

private:
    QDoubleSpinBox* m_spin = nullptr;
};

// A row with a text field (a password field with password set).
class EntryRow : public SettingRow
{
    Q_OBJECT

public:
    EntryRow(const QString& title, const QString& text, bool password = false, QWidget* parent = nullptr);

    [[nodiscard]] QString text() const;
    void setText(const QString& text);
    void setPlaceholder(const QString& text);
    [[nodiscard]] QLineEdit* lineEdit() const
    {
        return m_edit;
    }

signals:
    void textEdited(const QString& text);

private:
    QLineEdit* m_edit = nullptr;
};
