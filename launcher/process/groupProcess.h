#pragma once

#include <QByteArray>
#include <QObject>
#include <QProcess>
#include <QProcessEnvironment>
#include <QString>
#include <QStringList>

class QTimer;

// GroupProcess - a long-running program in its own process group (the game through run.sh, the
// FSR 4.1.1 build under Proton, The Hunter's Dream sign-in), its output line by line with stderr
// merged into stdout. stop() ends the whole group: run.sh's helpers and Proton's processes stop
// with it. Destroying a running GroupProcess stops it and waits for it.
class GroupProcess : public QObject
{
    Q_OBJECT

public:
    explicit GroupProcess(QObject* parent = nullptr);
    ~GroupProcess() override;

    // False when the program could not start (errorString() says why).
    bool start(const QString& program, const QStringList& arguments, const QProcessEnvironment& env,
               const QString& workingDirectory);

    // SIGTERM to the group, SIGKILL if it is still running a few seconds later.
    void stop();

    [[nodiscard]] bool isRunning() const;
    [[nodiscard]] QString errorString() const;

signals:
    void lineReceived(const QString& line);
    // exitCode is -1 when a signal ended the program.
    void finished(int exitCode);

private:
    // m_pid while no program runs.
    static constexpr qint64 NO_PROCESS = 0;

    void readOutput();
    void flushLines(bool all);
    void signalGroup(int signal);

    QProcess* m_process = nullptr;
    QTimer* m_killTimer = nullptr;
    QByteArray m_buffer;
    qint64 m_pid = NO_PROCESS;
};
