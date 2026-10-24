#include "logPage.h"

#include <QClipboard>
#include <QDateTime>
#include <QDir>
#include <QFileDialog>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSaveFile>
#include <QScrollBar>
#include <QTimer>
#include <QVBoxLayout>

#include "widgets/icons.h"

namespace
{
constexpr int MAX_LINES = 5000;
constexpr int FLUSH_INTERVAL_MS = 60;
constexpr int BAR_MARGIN = 10;
constexpr int VIEW_STRETCH = 1;
} // namespace

LogPage::LogPage(QWidget* parent) : QWidget(parent)
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_view = new QPlainTextEdit(this);
    m_view->setObjectName(QStringLiteral("logView"));
    m_view->setReadOnly(true);
    m_view->setMaximumBlockCount(MAX_LINES);
    m_view->setLineWrapMode(QPlainTextEdit::WidgetWidth);
    m_view->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    m_view->setPlaceholderText(tr("The game's output appears here while it runs."));
    layout->addWidget(m_view, VIEW_STRETCH);

    QWidget* bar = new QWidget(this);
    bar->setObjectName(QStringLiteral("logBar"));
    QHBoxLayout* barLayout = new QHBoxLayout(bar);
    barLayout->setContentsMargins(BAR_MARGIN, BAR_MARGIN, BAR_MARGIN, BAR_MARGIN);
    barLayout->addStretch();
    m_copyButton = new QPushButton(Icons::get(Icons::Kind::Copy), tr("Copy"), bar);
    m_exportButton = new QPushButton(Icons::get(Icons::Kind::Save), tr("Export log..."), bar);
    barLayout->addWidget(m_copyButton);
    barLayout->addWidget(m_exportButton);
    layout->addWidget(bar);

    connect(m_copyButton, &QPushButton::clicked, this, [this]()
    {
        QGuiApplication::clipboard()->setText(text());
        emit toast(tr("Log copied"));
    });
    connect(m_exportButton, &QPushButton::clicked, this, &LogPage::exportLog);

    m_flushTimer = new QTimer(this);
    m_flushTimer->setSingleShot(true);
    m_flushTimer->setInterval(FLUSH_INTERVAL_MS);
    connect(m_flushTimer, &QTimer::timeout, this, &LogPage::flush);
    updateButtons();
}

void LogPage::appendLine(const QString& line)
{
    m_pending.append(line);
    if (m_flushTimer->isActive() == false)
    {
        m_flushTimer->start();
    }
}

void LogPage::clear()
{
    m_pending.clear();
    m_view->clear();
    updateButtons();
}

QString LogPage::text() const
{
    QString all = m_view->toPlainText();
    if (m_pending.isEmpty() == false)
    {
        if (all.isEmpty() == false)
        {
            all += u'\n';
        }
        all += m_pending.join(u'\n');
    }
    return all;
}

void LogPage::flush()
{
    if (m_pending.isEmpty()) return;
    // Follows the end only when the reader is at the end already.
    QScrollBar* scrollBar = m_view->verticalScrollBar();
    const bool atEnd = scrollBar->value() == scrollBar->maximum();
    m_view->appendPlainText(m_pending.join(u'\n'));
    m_pending.clear();
    if (atEnd)
    {
        scrollBar->setValue(scrollBar->maximum());
    }
    updateButtons();
}

void LogPage::updateButtons()
{
    const bool hasText = m_view->document()->isEmpty() == false || m_pending.isEmpty() == false;
    m_copyButton->setEnabled(hasText);
    m_exportButton->setEnabled(hasText);
}

void LogPage::exportLog()
{
    const QString name = QStringLiteral("bbpc-%1.log").arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_HHmmss")));
    const QString path = QFileDialog::getSaveFileName(this, tr("Export log"), QDir::homePath() + u'/' + name,
                                                      tr("Logs (*.log *.txt)"));
    if (path.isEmpty()) return;
    QSaveFile file(path);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text) == false)
    {
        emit toast(tr("Could not save the log: %1").arg(file.errorString()));
        return;
    }
    file.write(text().toUtf8() + '\n');
    if (file.commit() == false)
    {
        emit toast(tr("Could not save the log: %1").arg(file.errorString()));
        return;
    }
    emit toast(tr("Log saved: %1").arg(path));
}
