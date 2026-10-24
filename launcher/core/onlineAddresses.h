#pragma once

#include <QJsonObject>
#include <QString>

// OnlineAddresses - the co-op servers' addresses, as scripts/online.py computes them. The game's
// own server (messages, bloodstains, ghosts) is the "community" server; bells and summons go
// through a shadNet server and its WebAPI. By default all three are Seamless Hunt's (BBSC's
// server). The Hunter's Dream is the other pool, signed in with Discord.
namespace OnlineAddresses
{
    inline const QString COMMUNITY_SERVER = QStringLiteral("http://play.seamlesshunt.com");
    inline const QString SHADNET_SERVER = QStringLiteral("play.seamlesshunt.com:31313");
    inline const QString WEBAPI_SERVER = QStringLiteral("http://play.seamlesshunt.com:31315");
    inline const QString HUNTERS_DREAM_SERVER = QStringLiteral("https://thehuntersdream.com");

    // The game server as scheme://host (the game picks the ports), Seamless Hunt by default.
    // Without a scheme: https, but http for the default server's host (it serves plain http).
    QString communityAddress(const QString& value);

    // The shadNet WebAPI of a server address host:port (the server's default port 31315).
    QString webapiAddress(const QString& server);

    // The WebAPI field, unless it holds the game server's address (a common mix-up): then the
    // shadNet server's own WebAPI.
    QString webapiSetting(const QString& webapi, const QString& server, const QString& community);

    // The Hunter's Dream's address as scheme://host (https unless a scheme is given).
    QString thdBase(const QString& value);

    // Saved addresses that were the defaults before 2026-10-15 (shadPS4's public shadNet, which
    // accepts only shadPS4's own builds, and The Hunter's Dream as the game server) become the
    // current defaults.
    void migrateOldDefaults(QJsonObject& settings);
}
