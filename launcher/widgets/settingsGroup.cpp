#include "settingsGroup.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QStyle>
#include <QToolButton>
#include <QVBoxLayout>

namespace
{
constexpr int GROUP_SPACING = 6;
constexpr int PAGE_MARGIN = 24;
constexpr int PAGE_SPACING = 26;
constexpr int COLUMN_MAX_WIDTH = 860;
constexpr int COLUMN_STRETCH = 1;
constexpr const char* FIRST_ROW_PROPERTY = "firstRow";

void repolish(QWidget* widget)
{
    widget->style()->unpolish(widget);
    widget->style()->polish(widget);
}
} // namespace

SettingsGroup::SettingsGroup(const QString& title, const QString& description, QWidget* parent) : QWidget(parent)
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(GROUP_SPACING);

    m_title = new QLabel(title, this);
    m_title->setObjectName(QStringLiteral("groupTitle"));
    m_title->setTextFormat(Qt::PlainText);
    m_title->setVisible(title.isEmpty() == false);
    layout->addWidget(m_title);
    m_description = new QLabel(this);
    m_description->setObjectName(QStringLiteral("groupDescription"));
    m_description->setTextFormat(Qt::PlainText);
    m_description->setWordWrap(true);
    layout->addWidget(m_description);
    setDescription(description);

    m_card = new QWidget(this);
    m_card->setObjectName(QStringLiteral("settingsCard"));
    m_card->setAttribute(Qt::WA_StyledBackground, true);
    m_rows = new QVBoxLayout(m_card);
    m_rows->setContentsMargins(0, 0, 0, 0);
    m_rows->setSpacing(0);
    layout->addWidget(m_card);
    m_card->hide();
}

void SettingsGroup::setDescription(const QString& description)
{
    m_description->setText(description);
    m_description->setVisible(description.isEmpty() == false);
}

void SettingsGroup::makeCollapsible(bool expanded)
{
    QToolButton* header = new QToolButton(this);
    header->setObjectName(QStringLiteral("groupHeader"));
    header->setText(m_title->text());
    header->setCheckable(true);
    header->setChecked(expanded);
    header->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    header->setArrowType(expanded ? Qt::DownArrow : Qt::RightArrow);
    header->setAutoRaise(true);
    header->setCursor(Qt::PointingHandCursor);
    static_cast<QVBoxLayout*>(layout())->insertWidget(0, header);
    m_title->hide();
    m_expanded = expanded;
    connect(header, &QToolButton::toggled, this, [this, header](bool on)
    {
        header->setArrowType(on ? Qt::DownArrow : Qt::RightArrow);
        m_expanded = on;
        updateCardVisibility();
    });
    updateCardVisibility();
}

void SettingsGroup::updateCardVisibility()
{
    m_card->setVisible(m_expanded && m_rowList.isEmpty() == false);
}

void SettingsGroup::addRow(QWidget* row)
{
    insertRow(static_cast<int>(m_rowList.size()), row);
}

void SettingsGroup::insertRow(int index, QWidget* row)
{
    row->setParent(m_card);
    m_rows->insertWidget(index, row);
    m_rowList.insert(index, row);
    markFirstRow();
}

void SettingsGroup::removeRow(QWidget* row)
{
    m_rows->removeWidget(row);
    m_rowList.removeOne(row);
    row->hide();
    row->deleteLater();
    markFirstRow();
}

int SettingsGroup::rowCount() const
{
    return static_cast<int>(m_rowList.size());
}

int SettingsGroup::indexOf(QWidget* row) const
{
    return static_cast<int>(m_rowList.indexOf(row));
}

void SettingsGroup::markFirstRow()
{
    for (int i = 0; i < m_rowList.size(); ++i)
    {
        QWidget* row = m_rowList.at(i);
        const bool first = i == 0;
        if (row->property(FIRST_ROW_PROPERTY).toBool() != first)
        {
            row->setProperty(FIRST_ROW_PROPERTY, first);
            repolish(row);
        }
    }
    updateCardVisibility();
}

SettingsPage::SettingsPage(QWidget* parent) : QScrollArea(parent)
{
    setWidgetResizable(true);
    setFrameShape(QFrame::NoFrame);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    QWidget* content = new QWidget(this);
    content->setObjectName(QStringLiteral("pageContent"));
    QHBoxLayout* centering = new QHBoxLayout(content);
    centering->setContentsMargins(PAGE_MARGIN, PAGE_MARGIN, PAGE_MARGIN, PAGE_MARGIN);

    QWidget* column = new QWidget(content);
    column->setMaximumWidth(COLUMN_MAX_WIDTH);
    m_column = new QVBoxLayout(column);
    m_column->setContentsMargins(0, 0, 0, 0);
    m_column->setSpacing(PAGE_SPACING);

    centering->addStretch();
    centering->addWidget(column, COLUMN_STRETCH);
    centering->addStretch();
    setWidget(content);
}

void SettingsPage::addGroup(QWidget* group)
{
    m_column->addWidget(group);
}

void SettingsPage::finish()
{
    m_column->addStretch();
}
