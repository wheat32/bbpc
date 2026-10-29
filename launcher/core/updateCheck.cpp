#include "updateCheck.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QVersionNumber>

QString UpdateCheck::remoteVersion(const QByteArray& json)
{
    const QJsonObject object = QJsonDocument::fromJson(json).object();
    return object.value(QStringLiteral("app_version")).toString().trimmed();
}

bool UpdateCheck::isNewer(const QString& remote, const QString& local)
{
    const QVersionNumber remoteNumber = QVersionNumber::fromString(remote);
    const QVersionNumber localNumber = QVersionNumber::fromString(local);
    if (remoteNumber.isNull() || localNumber.isNull()) return false;
    // Trailing zeros removed: QVersionNumber counts 1.0.0 as later than 1.0.
    return remoteNumber.normalized() > localNumber.normalized();
}
