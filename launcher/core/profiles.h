#pragma once

#include <QJsonObject>
#include <QList>
#include <QString>
#include <QStringList>

// Profiles - the mod and third-party patch selections run.sh applies at start (scripts/mods.py
// reads mods.json, scripts/patches.py patches.json, both in the data folder).
namespace Profiles
{
    // A mod folder and whether it loads. The list order is the load order: a mod lower in the list
    // wins when two replace the same file.
    struct Mod
    {
        QString name;
        bool enabled = true;
    };

    // The mods found (scripts/mods.py) in the saved order; new ones are appended, enabled, in the
    // order they were found (alphabetical).
    QList<Mod> arrangeMods(const QStringList& available, const QJsonObject& profile);

    // mods.json: {"order": [...], "disabled": [...]}.
    QJsonObject modsJson(const QList<Mod>& mods);

    // A patch of a shadPS4-format XML file. key is "<file name>/<patch name>"; defaultOn is the
    // file's own isEnabled.
    struct Patch
    {
        QString key;
        QString name;
        QString author;
        QString note;
        bool defaultOn = false;
        bool enabled = false;
    };

    // Sets enabled from patches.json: listed as enabled, or on by default and not listed as
    // disabled.
    void applyPatchProfile(QList<Patch>& patches, const QJsonObject& profile);

    // patches.json: the patches whose switch differs from their file's isEnabled. Choices for
    // patches not shown now (another folder) are kept.
    QJsonObject patchesJson(const QList<Patch>& shown, const QJsonObject& profile);

    QJsonObject readJson(const QString& path);
    bool writeJson(const QString& path, const QJsonObject& json);
}
