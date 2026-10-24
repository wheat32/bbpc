#pragma once

// debug.h - Lightweight diagnostic logging helpers.
// Prints to stdout when the launcher is started from a terminal. Every line is prefixed with a
// tag for easy grepping. The game's own output goes to the Log page, not here.

#include <QDateTime>
#include <QString>
#include <cstdio>

inline void dbgPrint(const char* tag, const QString& message)
{
    const QString timestamp = QDateTime::currentDateTime().toString(QStringLiteral("hh:mm:ss.zzz"));
    std::fprintf(stdout, "[%s] [%s] %s\n", qPrintable(timestamp), tag, qPrintable(message));
    std::fflush(stdout);
}

#define DBG_APP(msg)      dbgPrint("APP     ", (msg))
#define DBG_GAME(msg)     dbgPrint("GAME    ", (msg))
#define DBG_HELPER(msg)   dbgPrint("HELPER  ", (msg))
#define DBG_SETTINGS(msg) dbgPrint("SETTINGS", (msg))
