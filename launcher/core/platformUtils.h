#pragma once
// platformUtils.h
// Utilities for identifying the current packaging/runtime environment.

#include <QCoreApplication>
#include <QString>

#include "portPaths.h"

// Returns true when this process is running inside an AppImage (the AppImage runtime sets
// $APPIMAGE to the .AppImage file).
inline bool isRunningAsAppImage()
{
    return qEnvironmentVariableIsSet("APPIMAGE");
}

// Returns a human-readable string identifying the current packaging format: "AppImage",
// "Release tarball" (an installed prebuilt port), or "Source build" (the last two translated).
inline QString packageTypeName()
{
    if (isRunningAsAppImage())
    {
        return QStringLiteral("AppImage");
    }
    if (PortPaths::packaged())
    {
        return QCoreApplication::translate("PlatformUtils", "Release tarball");
    }
    return QCoreApplication::translate("PlatformUtils", "Source build");
}
