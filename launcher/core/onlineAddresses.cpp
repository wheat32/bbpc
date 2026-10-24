#include "onlineAddresses.h"

#include <QStringList>

namespace
{
const QString SCHEME_SEPARATOR = QStringLiteral("://");
const QString WEBAPI_PORT = QStringLiteral("31315");

struct OldDefault
{
    QString key;
    QStringList values;
    QString current;
};

const QList<OldDefault>& oldDefaults()
{
    static const QList<OldDefault> list = {
        { QStringLiteral("online_server"), { QStringLiteral("srv.shadps4.net:31313") }, OnlineAddresses::SHADNET_SERVER },
        { QStringLiteral("online_community"), { QStringLiteral("https://thehuntersdream.com"), QString() },
          OnlineAddresses::COMMUNITY_SERVER },
        { QStringLiteral("online_webapi"), { QString() }, OnlineAddresses::WEBAPI_SERVER },
    };
    return list;
}

// The host of an address without scheme, path and port.
QString hostOf(const QString& rest)
{
    return rest.section(u'/', 0, 0).section(u':', 0, 0);
}
} // namespace

QString OnlineAddresses::communityAddress(const QString& value)
{
    QString address = value.trimmed();
    if (address.isEmpty())
    {
        address = COMMUNITY_SERVER;
    }
    QString scheme;
    QString rest = address;
    const qsizetype separator = address.lastIndexOf(SCHEME_SEPARATOR);
    if (separator >= 0)
    {
        scheme = address.left(separator);
        rest = address.mid(separator + SCHEME_SEPARATOR.size());
    }
    const QString host = hostOf(rest);
    const qsizetype defaultSeparator = COMMUNITY_SERVER.indexOf(SCHEME_SEPARATOR);
    const QString defaultScheme = COMMUNITY_SERVER.left(defaultSeparator);
    const QString defaultHost = COMMUNITY_SERVER.mid(defaultSeparator + SCHEME_SEPARATOR.size());
    if (scheme.isEmpty())
    {
        scheme = host == defaultHost ? defaultScheme : QStringLiteral("https");
    }
    return scheme + SCHEME_SEPARATOR + host;
}

QString OnlineAddresses::webapiAddress(const QString& server)
{
    QString address = server.trimmed();
    if (address.isEmpty())
    {
        address = SHADNET_SERVER;
    }
    const QString host = address.count(u':') == 1 ? address.section(u':', 0, 0) : address;
    return QStringLiteral("http://") + host + u':' + WEBAPI_PORT;
}

QString OnlineAddresses::webapiSetting(const QString& webapi, const QString& server, const QString& community)
{
    QString field = webapi.trimmed();
    const QString communityFull = communityAddress(community);
    const QString communityHost = communityFull.mid(communityFull.indexOf(SCHEME_SEPARATOR) + SCHEME_SEPARATOR.size());
    if (field.isEmpty() == false && field.contains(communityHost))
    {
        field.clear();
    }
    if (field.isEmpty())
    {
        return webapiAddress(server);
    }
    return field;
}

QString OnlineAddresses::thdBase(const QString& value)
{
    QString address = value.trimmed();
    while (address.endsWith(u'/'))
    {
        address.chop(1);
    }
    if (address.isEmpty())
    {
        address = HUNTERS_DREAM_SERVER;
    }
    if (address.contains(SCHEME_SEPARATOR) == false)
    {
        address = QStringLiteral("https://") + address;
    }
    const qsizetype separator = address.indexOf(SCHEME_SEPARATOR);
    const QString scheme = address.left(separator);
    const QString rest = address.mid(separator + SCHEME_SEPARATOR.size());
    return scheme + SCHEME_SEPARATOR + rest.section(u'/', 0, 0);
}

void OnlineAddresses::migrateOldDefaults(QJsonObject& settings)
{
    for (const OldDefault& old : oldDefaults())
    {
        const QString saved = settings.value(old.key).toString().trimmed();
        if (old.values.contains(saved))
        {
            settings.insert(old.key, old.current);
        }
    }
}
