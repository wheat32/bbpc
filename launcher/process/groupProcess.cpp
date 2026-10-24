#include "groupProcess.h"

#include <QTimer>
#include <csignal>
#include <unistd.h>

#include "debug.h"

namespace
{
constexpr int START_TIMEOUT_MS = 10000;
constexpr int KILL_DELAY_MS = 3000;
constexpr int CRASH_EXIT_CODE = -1;
} // namespace

GroupProcess::GroupProcess(QObject* parent) : QObject(parent)
{
    m_process = new QProcess(this);
    m_process->setProcessChannelMode(QProcess::MergedChannels);
    // Its own session and process group: stop() reaches every process it starts.
    m_process->setChildProcessModifier([]()
    {
        ::setsid();
    });
    connect(m_process, &QProcess::readyReadStandardOutput, this, &GroupProcess::readOutput);
    connect(m_process, &QProcess::finished, this, [this](int exitCode, QProcess::ExitStatus status)
    {
        readOutput();
        flushLines(true);
        m_killTimer->stop();
        m_pid = NO_PROCESS;
        emit finished(status == QProcess::NormalExit ? exitCode : CRASH_EXIT_CODE);
    });

    m_killTimer = new QTimer(this);
    m_killTimer->setSingleShot(true);
    m_killTimer->setInterval(KILL_DELAY_MS);
    connect(m_killTimer, &QTimer::timeout, this, [this]()
    {
        signalGroup(SIGKILL);
    });
}

GroupProcess::~GroupProcess()
{
    // Nothing is reported while the owner is being destroyed.
    m_process->disconnect(this);
    if (isRunning() == false) return;
    signalGroup(SIGTERM);
    if (m_process->waitForFinished(KILL_DELAY_MS) == false)
    {
        signalGroup(SIGKILL);
        m_process->waitForFinished();
    }
}

bool GroupProcess::start(const QString& program, const QStringList& arguments, const QProcessEnvironment& env,
                         const QString& workingDirectory)
{
    if (isRunning()) return false;
    m_buffer.clear();
    m_process->setProcessEnvironment(env);
    m_process->setWorkingDirectory(workingDirectory);
    m_process->start(program, arguments);
    if (m_process->waitForStarted(START_TIMEOUT_MS) == false)
    {
        DBG_GAME(QStringLiteral("Could not start ") + program + QStringLiteral(": ") + m_process->errorString());
        return false;
    }
    m_pid = m_process->processId();
    DBG_GAME(QStringLiteral("Started ") + program + u' ' + arguments.join(u' ') + QStringLiteral(" (pid ")
             + QString::number(m_pid) + u')');
    return true;
}

void GroupProcess::stop()
{
    if (isRunning() == false) return;
    signalGroup(SIGTERM);
    m_killTimer->start();
}

bool GroupProcess::isRunning() const
{
    return m_process->state() != QProcess::NotRunning;
}

QString GroupProcess::errorString() const
{
    return m_process->errorString();
}

void GroupProcess::signalGroup(int signal)
{
    if (m_pid <= NO_PROCESS) return;
    if (::kill(static_cast<pid_t>(-m_pid), signal) == 0) return;
    // No group (it could not be created): the program alone.
    if (signal == SIGKILL)
    {
        m_process->kill();
    }
    else
    {
        m_process->terminate();
    }
}

void GroupProcess::readOutput()
{
    m_buffer.append(m_process->readAllStandardOutput());
    flushLines(false);
}

void GroupProcess::flushLines(bool all)
{
    qsizetype newline = m_buffer.indexOf('\n');
    while (newline >= 0)
    {
        QByteArray line = m_buffer.left(newline);
        m_buffer.remove(0, newline + 1);
        if (line.endsWith('\r'))
        {
            line.chop(1);
        }
        emit lineReceived(QString::fromUtf8(line));
        newline = m_buffer.indexOf('\n');
    }
    if (all && m_buffer.isEmpty() == false)
    {
        emit lineReceived(QString::fromUtf8(m_buffer));
        m_buffer.clear();
    }
}
