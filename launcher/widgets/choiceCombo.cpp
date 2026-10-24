#include "choiceCombo.h"

#include <QSignalBlocker>

namespace
{
constexpr int VALUE_ROLE = Qt::UserRole;
constexpr int NOTE_ROLE = Qt::UserRole + 1;
} // namespace

ChoiceCombo::ChoiceCombo(QWidget* parent) : QComboBox(parent)
{
    setSizeAdjustPolicy(QComboBox::AdjustToContents);
    connect(this, &QComboBox::currentIndexChanged, this, [this]()
    {
        emit valueChanged(value());
    });
}

void ChoiceCombo::setChoices(const QList<Choices::Choice>& choices)
{
    QList<Item> items;
    for (const Choices::Choice& choice : choices)
    {
        items.append({ Choices::label(choice), QString::fromLatin1(choice.value), Choices::note(choice) });
    }
    setItems(items);
}

void ChoiceCombo::setItems(const QList<Item>& items)
{
    const QString current = value();
    {
        const QSignalBlocker blocker(this);
        clear();
        for (const Item& item : items)
        {
            addItem(item.label);
            setItemData(count() - 1, item.value, VALUE_ROLE);
            setItemData(count() - 1, item.note, NOTE_ROLE);
            if (item.note.isEmpty() == false)
            {
                setItemData(count() - 1, item.note, Qt::ToolTipRole);
            }
        }
        const int index = findData(current, VALUE_ROLE);
        setCurrentIndex(index >= 0 ? index : 0);
    }
    emit valueChanged(value());
}

void ChoiceCombo::setValue(const QString& value)
{
    const int index = findData(value, VALUE_ROLE);
    const int target = index >= 0 ? index : 0;
    if (target == currentIndex())
    {
        return;
    }
    setCurrentIndex(target);
}

QString ChoiceCombo::value() const
{
    return currentIndex() >= 0 ? currentData(VALUE_ROLE).toString() : QString();
}

QString ChoiceCombo::note() const
{
    return currentIndex() >= 0 ? currentData(NOTE_ROLE).toString() : QString();
}

bool ChoiceCombo::hasValue(const QString& value) const
{
    return findData(value, VALUE_ROLE) >= 0;
}
