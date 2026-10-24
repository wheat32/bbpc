#include "processRunner.h"

#include <QPointer>
#include <QProcess>
#include <QTimer>

#include "debug.h"

QObject* ProcessRunner::run(const QString& program, const QStringList& arguments, const QProcessEnvironment& env,
                            int timeoutMs, QObject* context, Callback done)
{
    QProcess* process = new QProcess(context);
    process->setProcessEnvironment(env);
    QPointer<QObject> guard(context);
    // Exactly once: the program's end, or its failure to start.
    auto finish = [process, guard, done](bool started)
    {
        if (process->property("finished").toBool()) return;
        process->setProperty("finished", true);
        Result result;
        result.started = started;
        if (started)
        {
            result.exitCode = process->exitStatus() == QProcess::NormalExit ? process->exitCode() : Result::NO_EXIT_CODE;
            result.output = process->readAllStandardOutput();
            result.errors = process->readAllStandardError();
        }
        else
        {
            result.errors = process->errorString().toUtf8();
        }
        if (guard != nullptr && done != nullptr)
        {
            done(result);
        }
        process->deleteLater();
    };
    QObject::connect(process, &QProcess::finished, process, [finish]()
    {
        finish(true);
    });
    QObject::connect(process, &QProcess::errorOccurred, process, [process, finish](QProcess::ProcessError error)
    {
        if (error == QProcess::FailedToStart)
        {
            DBG_HELPER(QStringLiteral("Could not start ") + process->program() + QStringLiteral(": ") + process->errorString());
            finish(false);
        }
    });
    if (timeoutMs > NO_TIMEOUT)
    {
        QTimer::singleShot(timeoutMs, process, [process]()
        {
            process->kill();
        });
    }
    process->start(program, arguments);
    return process;
}

void ProcessRunner::cancel(QObject* handle)
{
    QProcess* process = qobject_cast<QProcess*>(handle);
    if (process != nullptr)
    {
        process->kill();
    }
}
