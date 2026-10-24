#include "portQueries.h"

#include <QCoreApplication>
#include <QFileInfo>
#include <QJsonDocument>

#include "core/hostEnvironment.h"
#include "core/portPaths.h"
#include "debug.h"
#include "processRunner.h"

namespace
{
// The answer is the last line that is a JSON object (the scripts print their progress to stderr).
QJsonObject lastObject(const QByteArray& output)
{
    const QList<QByteArray> lines = output.split('\n');
    for (qsizetype i = lines.size() - 1; i >= 0; --i)
    {
        const QJsonDocument document = QJsonDocument::fromJson(lines.at(i).trimmed());
        if (document.isObject())
        {
            return document.object();
        }
    }
    return {};
}
} // namespace

QString PortQueries::python()
{
    return PortPaths::python();
}

QStringList PortQueries::scriptArguments(const QStringList& arguments)
{
    return QStringList { PortPaths::portDir() + QStringLiteral("/scripts/launcher_query.py") } + arguments;
}

void PortQueries::query(const QStringList& arguments, QObject* context, std::function<void(const QJsonObject&)> done,
                        int timeoutMs)
{
    const QString command = arguments.isEmpty() ? QString() : arguments.first();
    ProcessRunner::run(python(), scriptArguments(arguments), HostEnvironment::host(), timeoutMs, context,
                       [command, done](const ProcessRunner::Result& result)
    {
        QJsonObject answer = lastObject(result.output);
        if (answer.isEmpty())
        {
            QString why = QString::fromUtf8(result.errors).trimmed();
            if (why.isEmpty())
            {
                why = QCoreApplication::translate("PortQueries", "no answer (exit code %1)").arg(result.exitCode);
            }
            DBG_HELPER(QStringLiteral("launcher_query.py ") + command + QStringLiteral(": ") + why);
            answer.insert(ERROR_KEY, why);
        }
        done(answer);
    });
}

QObject* PortQueries::capabilities(const QStringList& arguments, QObject* context, std::function<void(const QString&)> done,
                                   int timeoutMs)
{
    const QString tool = PortPaths::capabilitiesTool();
    if (QFileInfo(tool).isExecutable() == false)
    {
        DBG_HELPER(QStringLiteral("No ") + tool + QStringLiteral(" (build the port first)"));
        done({});
        return nullptr;
    }
    // The GPU list in Vulkan's own order: BB_GPU_ID from the shell must not reorder it.
    QProcessEnvironment env = HostEnvironment::host();
    env.remove(QStringLiteral("BB_GPU_ID"));
    return ProcessRunner::run(tool, arguments, env, timeoutMs, context, [done](const ProcessRunner::Result& result)
    {
        done(QString::fromUtf8(result.output));
    });
}
