#include "trophies.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QRegularExpression>
#include <QXmlStreamReader>
#include <algorithm>
#include <optional>

namespace
{
const QString TROPHY_DIR = QStringLiteral("trophy");
const QString UNLOCKED_FILE = QStringLiteral("unlocked.txt");
const QString XML_FILE = QStringLiteral("TROP.XML");
const QString JOURNAL_FILE = QStringLiteral("trophies.jsonl");
const QString CONF_START = QStringLiteral("<trophyconf");
const QString CONF_END = QStringLiteral("</trophyconf>");
const QString TROPHY_ELEMENT = QStringLiteral("trophy");
const QString NAME_ELEMENT = QStringLiteral("name");
const QString DETAIL_ELEMENT = QStringLiteral("detail");
const QString HIDDEN_YES = QStringLiteral("yes");
const QString TROPHY_ID_KEY = QStringLiteral("trophy_id");
const QString UNLOCKED_AT_KEY = QStringLiteral("unlocked_at");
const QChar COMMENT = u'#';
// The system language the game sees when the setting cannot be read: English.
constexpr int DEFAULT_LANGUAGE = 1;
// TROP_01.XML, TROPnnn.PNG: the widths of the numbers in the file names.
constexpr int LANGUAGE_DIGITS = 2;
constexpr int ICON_DIGITS = 3;
constexpr int DECIMAL = 10;
const QChar ZERO = u'0';
const QRegularExpression DIGITS(QStringLiteral("^\\d+$"));
const QRegularExpression KEY(QStringLiteral("^[0-9A-Fa-f]{%1}$").arg(Trophies::KEY_DIGITS));

QStringList readLines(const QString& path)
{
    QFile file(path);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text) == false)
    {
        return {};
    }
    return QString::fromUtf8(file.readAll()).split(u'\n');
}

QString readText(const QString& path)
{
    QFile file(path);
    if (file.open(QIODevice::ReadOnly) == false)
    {
        return {};
    }
    return QString::fromUtf8(file.readAll());
}

bool isTrophyFolder(const QFileInfo& folder)
{
    const QDir dir(folder.absoluteFilePath());
    return QFileInfo(dir.filePath(UNLOCKED_FILE)).isFile() || QFileInfo(dir.filePath(XML_FILE)).isFile();
}

// A trophy id from trophies.jsonl: a number, or a number written as text.
std::optional<int> trophyId(const QJsonValue& value)
{
    if (value.isDouble())
    {
        return value.toInt();
    }
    if (value.isString())
    {
        bool ok = false;
        const int id = value.toString().trimmed().toInt(&ok);
        if (ok)
        {
            return id;
        }
    }
    return std::nullopt;
}
} // namespace

QString Trophies::trophyFolder(const QString& userDir)
{
    const QDir root(userDir + u'/' + TROPHY_DIR);
    if (root.exists() == false)
    {
        return {};
    }
    QFileInfoList folders;
    for (const QFileInfo& entry : root.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot))
    {
        if (isTrophyFolder(entry) == false) continue;
        if (entry.fileName() == BLOODBORNE_NP_ID)
        {
            return entry.absoluteFilePath();
        }
        folders.append(entry);
    }
    if (folders.isEmpty())
    {
        return {};
    }
    const auto newest = std::max_element(folders.cbegin(), folders.cend(), [](const QFileInfo& a, const QFileInfo& b)
    {
        return a.lastModified() < b.lastModified();
    });
    return newest->absoluteFilePath();
}

QList<Trophies::Trophy> Trophies::parseXml(const QString& text)
{
    const qsizetype start = text.indexOf(CONF_START);
    if (start < 0)
    {
        return {};
    }
    const qsizetype end = text.lastIndexOf(CONF_END);
    const QString body = end > start ? text.mid(start, end - start + CONF_END.size()) : text.mid(start);
    QList<Trophy> trophies;
    QXmlStreamReader reader(body);
    while (reader.atEnd() == false)
    {
        reader.readNext();
        if (reader.isStartElement() == false || reader.name() != TROPHY_ELEMENT) continue;
        const QXmlStreamAttributes attributes = reader.attributes();
        bool idOk = false;
        Trophy trophy;
        trophy.id = attributes.value(QStringLiteral("id")).toInt(&idOk);
        const QStringView grade = attributes.value(QStringLiteral("ttype"));
        trophy.grade = grade.isEmpty() ? UNKNOWN_GRADE : grade.front();
        trophy.hidden = attributes.value(QStringLiteral("hidden")) == HIDDEN_YES;
        while (reader.readNextStartElement())
        {
            if (reader.name() == NAME_ELEMENT)
            {
                trophy.name = reader.readElementText(QXmlStreamReader::IncludeChildElements).trimmed();
            }
            else if (reader.name() == DETAIL_ELEMENT)
            {
                trophy.detail = reader.readElementText(QXmlStreamReader::IncludeChildElements).trimmed();
            }
            else
            {
                reader.skipCurrentElement();
            }
        }
        if (idOk)
        {
            trophies.append(trophy);
        }
    }
    if (reader.hasError())
    {
        return {};
    }
    return trophies;
}

QHash<int, QString> Trophies::unlockTimes(const QString& userDir)
{
    QHash<int, QString> times;
    for (const QString& line : readLines(userDir + u'/' + JOURNAL_FILE))
    {
        const QJsonDocument document = QJsonDocument::fromJson(line.toUtf8());
        if (document.isObject() == false) continue;
        const QJsonObject event = document.object();
        const std::optional<int> id = trophyId(event.value(TROPHY_ID_KEY));
        const QJsonValue stamp = event.value(UNLOCKED_AT_KEY);
        if (id.has_value() == false || stamp.isUndefined() || stamp.isNull()) continue;
        const QString text = stamp.isString() ? stamp.toString() : stamp.toVariant().toString();
        if (times.contains(id.value()) == false || text < times.value(id.value()))
        {
            times.insert(id.value(), text);
        }
    }
    return times;
}

QSet<int> Trophies::unlockedIds(const QString& folder)
{
    QSet<int> ids;
    for (const QString& line : readLines(folder + u'/' + UNLOCKED_FILE))
    {
        const QString text = line.trimmed();
        if (text.isEmpty() || text.startsWith(COMMENT) || DIGITS.match(text).hasMatch() == false) continue;
        ids.insert(text.toInt());
    }
    return ids;
}

Trophies::Content Trophies::load(const QString& userDir, const QString& language)
{
    Content content;
    content.folder = trophyFolder(userDir);
    const QHash<int, QString> times = unlockTimes(userDir);
    QSet<int> unlocked = content.folder.isEmpty() ? QSet<int>() : unlockedIds(content.folder);
    for (auto it = times.cbegin(); it != times.cend(); ++it)
    {
        unlocked.insert(it.key());
    }

    QList<Trophy> known;
    if (content.folder.isEmpty() == false)
    {
        bool ok = false;
        int number = language.trimmed().toInt(&ok);
        if (ok == false)
        {
            number = DEFAULT_LANGUAGE;
        }
        const QStringList names = { QStringLiteral("TROP_%1.XML").arg(number, LANGUAGE_DIGITS, DECIMAL, ZERO), XML_FILE };
        for (const QString& name : names)
        {
            const QString path = content.folder + u'/' + name;
            if (QFileInfo(path).isFile() == false) continue;
            known = parseXml(readText(path));
            if (known.isEmpty() == false) break;
        }
    }
    content.named = known.isEmpty() == false;
    if (content.named == false)
    {
        QList<int> ids(unlocked.cbegin(), unlocked.cend());
        std::sort(ids.begin(), ids.end());
        for (const int id : ids)
        {
            Trophy trophy;
            trophy.id = id;
            known.append(trophy);
        }
    }

    for (Trophy trophy : known)
    {
        trophy.unlocked = unlocked.contains(trophy.id);
        trophy.time = times.value(trophy.id);
        if (content.folder.isEmpty() == false)
        {
            const QString icon = content.folder + QStringLiteral("/TROP%1.PNG").arg(trophy.id, ICON_DIGITS, DECIMAL, ZERO);
            if (QFileInfo(icon).isFile())
            {
                trophy.icon = icon;
            }
        }
        GradeCount& count = content.grades[trophy.grade];
        ++count.total;
        if (trophy.unlocked)
        {
            ++count.unlocked;
            ++content.unlocked;
        }
        content.trophies.append(trophy);
    }
    content.total = static_cast<int>(content.trophies.size());
    return content;
}

QDateTime Trophies::unlockTime(const QString& stamp)
{
    const QDateTime moment = QDateTime::fromString(stamp.trimmed(), Qt::ISODate);
    return moment.isValid() ? moment.toLocalTime() : QDateTime();
}

bool Trophies::validKey(const QString& text)
{
    return KEY.match(text.trimmed()).hasMatch();
}
