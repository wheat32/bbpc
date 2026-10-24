#include "profiles.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSaveFile>
#include <QSet>
#include <algorithm>

namespace
{
const QString ORDER = QStringLiteral("order");
const QString DISABLED = QStringLiteral("disabled");
const QString ENABLED = QStringLiteral("enabled");

QStringList strings(const QJsonObject& json, const QString& key)
{
    QStringList list;
    for (const QJsonValue& value : json.value(key).toArray())
    {
        if (value.isString())
        {
            list.append(value.toString());
        }
    }
    return list;
}

QJsonArray sortedArray(const QSet<QString>& set)
{
    QStringList list(set.begin(), set.end());
    std::sort(list.begin(), list.end());
    return QJsonArray::fromStringList(list);
}
} // namespace

QList<Profiles::Mod> Profiles::arrangeMods(const QStringList& available, const QJsonObject& profile)
{
    const QStringList disabled = strings(profile, DISABLED);
    QStringList order;
    for (const QString& name : strings(profile, ORDER) + available)
    {
        if (available.contains(name) && order.contains(name) == false)
        {
            order.append(name);
        }
    }
    QList<Mod> mods;
    for (const QString& name : order)
    {
        mods.append({ name, disabled.contains(name) == false });
    }
    return mods;
}

QJsonObject Profiles::modsJson(const QList<Mod>& mods)
{
    QStringList order;
    QStringList disabled;
    for (const Mod& mod : mods)
    {
        order.append(mod.name);
        if (mod.enabled == false)
        {
            disabled.append(mod.name);
        }
    }
    return { { ORDER, QJsonArray::fromStringList(order) }, { DISABLED, QJsonArray::fromStringList(disabled) } };
}

void Profiles::applyPatchProfile(QList<Patch>& patches, const QJsonObject& profile)
{
    const QStringList enabled = strings(profile, ENABLED);
    const QStringList disabled = strings(profile, DISABLED);
    for (Patch& patch : patches)
    {
        patch.enabled = enabled.contains(patch.key) || (patch.defaultOn && disabled.contains(patch.key) == false);
    }
}

QJsonObject Profiles::patchesJson(const QList<Patch>& shown, const QJsonObject& profile)
{
    QSet<QString> shownKeys;
    for (const Patch& patch : shown)
    {
        shownKeys.insert(patch.key);
    }
    QSet<QString> enabled;
    QSet<QString> disabled;
    for (const QString& key : strings(profile, ENABLED))
    {
        if (shownKeys.contains(key) == false)
        {
            enabled.insert(key);
        }
    }
    for (const QString& key : strings(profile, DISABLED))
    {
        if (shownKeys.contains(key) == false)
        {
            disabled.insert(key);
        }
    }
    for (const Patch& patch : shown)
    {
        if (patch.enabled == patch.defaultOn) continue;
        if (patch.enabled)
        {
            enabled.insert(patch.key);
        }
        else
        {
            disabled.insert(patch.key);
        }
    }
    return { { ENABLED, sortedArray(enabled) }, { DISABLED, sortedArray(disabled) } };
}

QJsonObject Profiles::readJson(const QString& path)
{
    QFile file(path);
    if (file.open(QIODevice::ReadOnly) == false)
    {
        return {};
    }
    return QJsonDocument::fromJson(file.readAll()).object();
}

bool Profiles::writeJson(const QString& path, const QJsonObject& json)
{
    QDir().mkpath(QFileInfo(path).absolutePath());
    QSaveFile file(path);
    if (file.open(QIODevice::WriteOnly) == false) return false;
    file.write(QJsonDocument(json).toJson(QJsonDocument::Indented));
    return file.commit();
}
