#pragma once

#include <QByteArray>
#include <QString>

// UpdateCheck - whether a newer bbpc is out. The launcher reads the version.json of the main
// branch (the release workflow publishes every push to main as release v<app_version>) a few
// seconds after it starts, and offers the releases page when that version is newer than its own.
namespace UpdateCheck
{
    inline const QString VERSION_URL = QStringLiteral("https://raw.githubusercontent.com/wheat32/bbpc/main/version.json");
    inline const QString RELEASES_URL = QStringLiteral("https://github.com/wheat32/bbpc/releases/latest");

    // The app_version of a version.json; an empty string when there is none.
    [[nodiscard]] QString remoteVersion(const QByteArray& json);

    // Whether remote is a later version than local (1.0 and 1.0.0 are the same); false when
    // either is not a version.
    [[nodiscard]] bool isNewer(const QString& remote, const QString& local);
}
