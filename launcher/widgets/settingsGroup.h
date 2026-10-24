#pragma once

#include <QList>
#include <QScrollArea>
#include <QString>
#include <QWidget>

class QLabel;
class QVBoxLayout;

// SettingsGroup - a titled card of setting rows, with an optional description under the title.
class SettingsGroup : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsGroup(const QString& title = {}, const QString& description = {}, QWidget* parent = nullptr);

    void setDescription(const QString& description);

    // The title becomes a button that shows and hides the rows (long lists).
    void makeCollapsible(bool expanded);

    void addRow(QWidget* row);
    void insertRow(int index, QWidget* row);
    // Removes and deletes the row.
    void removeRow(QWidget* row);
    [[nodiscard]] int rowCount() const;
    [[nodiscard]] int indexOf(QWidget* row) const;

private:
    // The first row has no separator above it.
    void markFirstRow();

    void updateCardVisibility();

    QLabel* m_title = nullptr;
    QLabel* m_description = nullptr;
    QWidget* m_card = nullptr;
    bool m_expanded = true;
    QVBoxLayout* m_rows = nullptr;
    QList<QWidget*> m_rowList;
};

// SettingsPage - a scrolling column of settings groups, kept to a readable width.
class SettingsPage : public QScrollArea
{
    Q_OBJECT

public:
    explicit SettingsPage(QWidget* parent = nullptr);

    void addGroup(QWidget* group);
    void finish();

private:
    QVBoxLayout* m_column = nullptr;
};
