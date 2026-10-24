#pragma once

#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QStringList>
#include <functional>

#include "processRunner.h"

// PortQueries - the launcher's questions to the port, answered without blocking the window:
// scripts/launcher_query.py (the game check, mods, patches, the FSR 4.1.1 DLL check, the co-op
// servers: the same code run.sh uses) and bb-gpu-capabilities (GPUs, monitors, gamepads, keys).
namespace PortQueries
{
    inline const QString ERROR_KEY = QStringLiteral("error");

    // done gets the script's JSON answer, or {"error": "<why>"} when it gave none.
    void query(const QStringList& arguments, QObject* context, std::function<void(const QJsonObject&)> done,
               int timeoutMs = ProcessRunner::NO_TIMEOUT);

    // The script and its arguments for a long-running query (thd-sign-in) run by the caller.
    QString python();
    QStringList scriptArguments(const QStringList& arguments);

    // done gets the tool's stdout (empty when it is missing). Returns a handle for
    // ProcessRunner::cancel (an input assignment waiting for a key).
    QObject* capabilities(const QStringList& arguments, QObject* context, std::function<void(const QString&)> done,
                          int timeoutMs);
}
