#pragma once

#include <QByteArray>
#include <QObject>
#include <QProcessEnvironment>
#include <QString>
#include <QStringList>
#include <functional>

// ProcessRunner - runs a short program without blocking the window (bb-gpu-capabilities, the
// port's Python queries) and hands its output to a callback. The callback runs only while context
// is alive; a program still running after timeoutMs (NO_TIMEOUT: no limit) is killed.
namespace ProcessRunner
{
    // A timeoutMs that lets the program run as long as it takes.
    inline constexpr int NO_TIMEOUT = 0;

    struct Result
    {
        // A program that did not start, or that a signal ended.
        static constexpr int NO_EXIT_CODE = -1;

        bool started = false;
        int exitCode = NO_EXIT_CODE;
        QByteArray output;
        QByteArray errors;
    };

    using Callback = std::function<void(const Result&)>;

    // Returns a handle that cancel() takes to stop the program early (its callback still runs).
    QObject* run(const QString& program, const QStringList& arguments, const QProcessEnvironment& env,
                 int timeoutMs, QObject* context, Callback done);

    void cancel(QObject* handle);
}
