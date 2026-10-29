#pragma once

#include <QChar>
#include <QDateTime>
#include <QHash>
#include <QList>
#include <QSet>
#include <QString>

// Trophies - the game's trophies as the port keeps them in the saves folder (docs/trophies.md):
// gpu/shim/bbport_trophies.cpp keeps <user>/trophy/<NP communication ID>/ with unlocked.txt (the
// ids unlocked, one a line) and, once the game has run with the player's trophy key, the decrypted
// TROP.XML / TROP_XX.XML (XX: the system language) with the TROPnnn.PNG icons;
// src/runtime_trophy.c appends every unlock with its time to <user>/trophies.jsonl. Without the
// XML only the unlocked trophies are known, by number.
namespace Trophies
{
    inline const QString BLOODBORNE_NP_ID = QStringLiteral("NPWR05818_00");
    // The trophy key's length in hexadecimal digits (BB_TROPHY_KEY).
    inline constexpr int KEY_DIGITS = 32;
    // The grades as TROP.XML's ttype names them, best first; UNKNOWN_GRADE without the XML.
    inline constexpr QChar PLATINUM = u'P';
    inline constexpr QChar GOLD = u'G';
    inline constexpr QChar SILVER = u'S';
    inline constexpr QChar BRONZE = u'B';
    inline constexpr QChar UNKNOWN_GRADE = u'?';

    struct Trophy
    {
        int id = 0;
        QChar grade = UNKNOWN_GRADE;
        bool hidden = false;
        QString name;
        QString detail;
        bool unlocked = false;
        // The first unlock's time from trophies.jsonl ("2026-10-26T21:05:00Z"), or empty.
        QString time;
        // TROPnnn.PNG when the game has decrypted it, else empty.
        QString icon;
    };

    struct GradeCount
    {
        int unlocked = 0;
        int total = 0;
    };

    struct Content
    {
        // The trophy folder, empty when the game has kept no trophy yet.
        QString folder;
        // The names are known (the game decrypted TROP.XML with the trophy key).
        bool named = false;
        QList<Trophy> trophies;
        int unlocked = 0;
        int total = 0;
        QHash<QChar, GradeCount> grades;
    };

    // <user>/trophy/<NP communication ID> of the game: Bloodborne's when it is there, else the
    // folder written last; empty when the game has not kept any trophy yet.
    QString trophyFolder(const QString& userDir);

    // The trophies of a TROP.XML text in its order. The file may begin with a signature comment
    // or other bytes before the XML; a damaged file gives none.
    QList<Trophy> parseXml(const QString& text);

    // The first unlock time of each trophy id in <user>/trophies.jsonl.
    QHash<int, QString> unlockTimes(const QString& userDir);

    // The ids in <folder>/unlocked.txt.
    QSet<int> unlockedIds(const QString& folder);

    // Everything the Trophies page shows. language: the PS4 system language the game sees
    // (BB_LANGUAGE: 1 English, 8 Russian), whose TROP_XX.XML is read first.
    Content load(const QString& userDir, const QString& language);

    // An unlock time as a local date and time (invalid when it cannot be read).
    QDateTime unlockTime(const QString& stamp);

    // Whether text is a trophy key: 32 hexadecimal digits, spaces around it aside.
    bool validKey(const QString& text);
}
