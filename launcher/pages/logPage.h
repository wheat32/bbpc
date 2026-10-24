#pragma once

#include <QStringList>
#include <QWidget>

class QPlainTextEdit;
class QPushButton;
class QTimer;

// LogPage - the output of the game (run.sh) and of the FSR 4.1.1 build, with copy and export. The
// last MAX_LINES lines are kept; lines arrive in batches so that a chatty game does not slow the
// window down.
class LogPage : public QWidget
{
    Q_OBJECT

public:
    explicit LogPage(QWidget* parent = nullptr);

    void appendLine(const QString& line);
    void clear();
    [[nodiscard]] QString text() const;

signals:
    void toast(const QString& message);

private:
    void flush();
    void updateButtons();
    void exportLog();

    QPlainTextEdit* m_view = nullptr;
    QPushButton* m_copyButton = nullptr;
    QPushButton* m_exportButton = nullptr;
    QTimer* m_flushTimer = nullptr;
    QStringList m_pending;
};
