#pragma once

#include <QComboBox>
#include <QList>
#include <QString>

#include "core/choices.h"

// ChoiceCombo - a drop-down of labeled values, each with an optional note explaining it.
class ChoiceCombo : public QComboBox
{
    Q_OBJECT

public:
    struct Item
    {
        QString label;
        QString value;
        QString note;
    };

    explicit ChoiceCombo(QWidget* parent = nullptr);

    void setChoices(const QList<Choices::Choice>& choices);
    // Replaces the items, keeping the selection by value when it is still listed.
    void setItems(const QList<Item>& items);

    // Selects value; the first item when it is not listed.
    void setValue(const QString& value);
    [[nodiscard]] QString value() const;
    [[nodiscard]] QString note() const;
    [[nodiscard]] bool hasValue(const QString& value) const;

signals:
    // The user's choice or setValue() changed the selection.
    void valueChanged(const QString& value);
};
