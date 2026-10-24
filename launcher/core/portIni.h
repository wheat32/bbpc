#pragma once

#include <QHash>
#include <QSet>
#include <QString>
#include <QStringList>
#include <optional>

// PortIni - bbport.ini, the port's own settings (the in-game menu edits the same file: Insert or
// L3+R3). The launcher edits some keys and keeps the rest of the file as it is: an edited key is
// rewritten in place, a new one is appended, comments and unknown keys stay. A key removed with
// remove() is dropped from the file (a control binding back to its default).
class PortIni
{
public:
    // The keys the launcher edits and their defaults (the game's effects included).
    static const QList<QPair<QString, QString>>& defaults();

    // The file's values over the defaults; a missing file gives the defaults alone.
    void load(const QString& path);
    void parse(const QString& text);

    bool save(const QString& path) const;
    [[nodiscard]] QString serialize() const;

    [[nodiscard]] QString value(const QString& key, const QString& fallback = {}) const;
    [[nodiscard]] bool contains(const QString& key) const;
    void setValue(const QString& key, const QString& value);
    void remove(const QString& key);

    // Every key known (defaults first), its value (none: removed), and whether the parsed file has
    // a line for it.
    [[nodiscard]] const QStringList& keys() const
    {
        return m_order;
    }
    [[nodiscard]] std::optional<QString> raw(const QString& key) const;
    [[nodiscard]] bool inFile(const QString& key) const;

    // Writes into the file at path only the keys whose value differs between previous and
    // current (the launcher's own edits), and the keys the file lacks; whatever else is in the
    // file (the in-game menu's changes) stays. Returns false when the file could not be written.
    static bool saveChanges(const QString& path, const PortIni& previous, const PortIni& current);

private:
    // Insertion order (defaults first, then the file's own keys) for the keys appended on save.
    QStringList m_order;
    QHash<QString, std::optional<QString>> m_values;
    QStringList m_lines;
    QSet<QString> m_fileKeys;
};
