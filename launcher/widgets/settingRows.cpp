#include "settingRows.h"

#include <QAction>
#include <QDoubleSpinBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QVBoxLayout>

#include "choiceCombo.h"
#include "toggleSwitch.h"

namespace
{
constexpr int ROW_MARGIN_H = 14;
constexpr int ROW_MARGIN_V = 9;
constexpr int ROW_SPACING = 12;
constexpr int TEXT_SPACING = 2;
constexpr int SUFFIX_SPACING = 4;
constexpr int TEXT_STRETCH = 1;
constexpr int ROW_MIN_HEIGHT = 50;
constexpr int ENTRY_MIN_WIDTH = 300;
constexpr int ENTRY_STRETCH = 1;
constexpr int SPIN_MIN_WIDTH = 110;
} // namespace

SettingRow::SettingRow(const QString& title, const QString& subtitle, QWidget* parent) : QFrame(parent)
{
    setObjectName(QStringLiteral("settingRow"));
    setMinimumHeight(ROW_MIN_HEIGHT);

    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(ROW_MARGIN_H, ROW_MARGIN_V, ROW_MARGIN_H, ROW_MARGIN_V);
    layout->setSpacing(ROW_SPACING);

    m_prefix = new QHBoxLayout();
    m_prefix->setSpacing(SUFFIX_SPACING);
    layout->addLayout(m_prefix);

    QVBoxLayout* text = new QVBoxLayout();
    text->setSpacing(TEXT_SPACING);
    m_title = new QLabel(title, this);
    m_title->setObjectName(QStringLiteral("rowTitle"));
    m_title->setTextFormat(Qt::PlainText);
    m_title->setWordWrap(true);
    m_subtitle = new QLabel(this);
    m_subtitle->setObjectName(QStringLiteral("rowSubtitle"));
    m_subtitle->setTextFormat(Qt::PlainText);
    m_subtitle->setWordWrap(true);
    text->addWidget(m_title);
    text->addWidget(m_subtitle);
    layout->addLayout(text, TEXT_STRETCH);

    m_suffix = new QHBoxLayout();
    m_suffix->setSpacing(SUFFIX_SPACING);
    layout->addLayout(m_suffix);

    setSubtitle(subtitle);
}

void SettingRow::setTitle(const QString& title)
{
    m_title->setText(title);
}

QString SettingRow::title() const
{
    return m_title->text();
}

void SettingRow::setSubtitle(const QString& subtitle)
{
    m_subtitle->setText(subtitle);
    m_subtitle->setVisible(subtitle.isEmpty() == false);
}

QString SettingRow::subtitle() const
{
    return m_subtitle->text();
}

void SettingRow::addPrefix(QWidget* widget)
{
    widget->setParent(this);
    m_prefix->addWidget(widget, 0, Qt::AlignVCenter);
}

void SettingRow::addSuffix(QWidget* widget, int stretch)
{
    widget->setParent(this);
    m_suffix->addWidget(widget, stretch, Qt::AlignVCenter);
}

void SettingRow::removeWidget(QWidget* widget)
{
    m_prefix->removeWidget(widget);
    m_suffix->removeWidget(widget);
    widget->deleteLater();
}

SwitchRow::SwitchRow(const QString& title, const QString& subtitle, bool on, QWidget* parent)
    : SettingRow(title, subtitle, parent)
{
    m_switch = new ToggleSwitch(this);
    m_switch->setOn(on, false);
    addSuffix(m_switch);
    connect(m_switch, &ToggleSwitch::toggled, this, &SwitchRow::toggled);
}

bool SwitchRow::isOn() const
{
    return m_switch->isOn();
}

void SwitchRow::setOn(bool on)
{
    if (m_switch->isOn() == on) return;
    m_switch->setOn(on);
    emit toggled(on);
}

void SwitchRow::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && isEnabled() && rect().contains(event->position().toPoint()))
    {
        setOn(isOn() == false);
    }
    SettingRow::mouseReleaseEvent(event);
}

ChoiceRow::ChoiceRow(const QString& title, const QString& subtitle, QWidget* parent)
    : SettingRow(title, subtitle, parent), m_baseSubtitle(subtitle)
{
    m_combo = new ChoiceCombo(this);
    addSuffix(m_combo);
    connect(m_combo, &ChoiceCombo::valueChanged, this, [this](const QString& value)
    {
        showNote();
        emit valueChanged(value);
    });
}

ChoiceRow::ChoiceRow(const QString& title, const QString& subtitle, const QList<Choices::Choice>& choices,
                     const QString& value, QWidget* parent)
    : ChoiceRow(title, subtitle, parent)
{
    m_combo->setChoices(choices);
    m_combo->setValue(value);
    showNote();
}

QString ChoiceRow::value() const
{
    return m_combo->value();
}

void ChoiceRow::setBaseSubtitle(const QString& subtitle)
{
    m_baseSubtitle = subtitle;
    showNote();
}

void ChoiceRow::showNote()
{
    QStringList lines;
    if (m_baseSubtitle.isEmpty() == false)
    {
        lines.append(m_baseSubtitle);
    }
    if (m_combo->note().isEmpty() == false)
    {
        lines.append(m_combo->note());
    }
    setSubtitle(lines.join(u'\n'));
}

SpinRow::SpinRow(const QString& title, const QString& subtitle, double minimum, double maximum, double step,
                 int decimals, double value, QWidget* parent)
    : SettingRow(title, subtitle, parent)
{
    m_spin = new QDoubleSpinBox(this);
    m_spin->setRange(minimum, maximum);
    m_spin->setSingleStep(step);
    m_spin->setDecimals(decimals);
    m_spin->setValue(value);
    m_spin->setMinimumWidth(SPIN_MIN_WIDTH);
    addSuffix(m_spin);
    connect(m_spin, &QDoubleSpinBox::valueChanged, this, &SpinRow::valueChanged);
}

double SpinRow::value() const
{
    return m_spin->value();
}

void SpinRow::setValue(double value)
{
    m_spin->setValue(value);
}

EntryRow::EntryRow(const QString& title, const QString& text, bool password, QWidget* parent)
    : SettingRow(title, {}, parent)
{
    m_edit = new QLineEdit(text, this);
    m_edit->setMinimumWidth(ENTRY_MIN_WIDTH);
    m_edit->setClearButtonEnabled(password == false);
    if (password)
    {
        m_edit->setEchoMode(QLineEdit::Password);
        QAction* reveal = m_edit->addAction(QIcon(QStringLiteral(":/assets/eye.svg")), QLineEdit::TrailingPosition);
        reveal->setCheckable(true);
        reveal->setToolTip(tr("Show the password"));
        connect(reveal, &QAction::toggled, m_edit, [this](bool shown)
        {
            m_edit->setEchoMode(shown ? QLineEdit::Normal : QLineEdit::Password);
        });
    }
    addSuffix(m_edit, ENTRY_STRETCH);
    connect(m_edit, &QLineEdit::textEdited, this, &EntryRow::textEdited);
}

QString EntryRow::text() const
{
    return m_edit->text();
}

void EntryRow::setText(const QString& text)
{
    m_edit->setText(text);
}

void EntryRow::setPlaceholder(const QString& text)
{
    m_edit->setPlaceholderText(text);
}
