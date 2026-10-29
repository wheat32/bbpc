#include <QtTest/QtTest>

#include "trophies.h"

namespace
{
// A decrypted TROP.XML as the game leaves it: a signature comment before the XML declaration.
const QString XML = QStringLiteral(
    "<!--Sce-Np-Trophy-Signature: abc-->\n"
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<trophyconf version=\"1.1\" platform=\"ps4\">\n"
    " <title-name>Test</title-name>\n"
    " <trophy id=\"000\" hidden=\"no\" ttype=\"P\" pid=\"-1\"><name>All of it</name><detail>Everything.</detail></trophy>\n"
    " <trophy id=\"001\" hidden=\"yes\" ttype=\"G\" pid=\"000\"><name>Fish &amp; Chips</name><detail>Secret.</detail></trophy>\n"
    " <trophy id=\"002\" hidden=\"no\" ttype=\"B\" pid=\"000\"><name>Bronze</name><detail>Small.</detail></trophy>\n"
    "</trophyconf>\n");
const QString JOURNAL = QStringLiteral(
    "{\"v\":1,\"title_id\":\"CUSA03173\",\"trophy_id\":2,\"unlocked_at\":\"2026-10-26T21:05:00Z\"}\n"
    "{\"v\":1,\"title_id\":\"CUSA03173\",\"trophy_id\":2,\"unlocked_at\":\"2026-10-27T10:00:00Z\"}\n"
    "{\"v\":1,\"title_id\":\"CUSA03173\",\"trophy_id\":1,\"unlocked_at\":\"2026-10-26T22:00:00Z\"}\n"
    "not json\n");
constexpr int FIRST_TROPHY = 0;
constexpr int SECOND_TROPHY = 1;
constexpr int SECRET_ID = 1;
constexpr int BRONZE_ID = 2;
constexpr int XML_TROPHIES = 3;
constexpr int UNLOCKED_TROPHIES = 2;

void writeText(const QString& path, const QString& text)
{
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(text.toUtf8());
}
} // namespace

// The Trophies page's content on synthetic files (no game files, no key), as the original
// launcher's tests/test_trophy_list.py checks it.
class TstTrophies : public QObject
{
    Q_OBJECT

private slots:
    void load_nothingKept_isEmpty()
    {
        QTemporaryDir user;
        const Trophies::Content content = Trophies::load(user.path(), QStringLiteral("1"));
        QVERIFY(content.folder.isEmpty());
        QCOMPARE(content.total, 0);
        QVERIFY(content.named == false);
    }

    void load_withoutTheKey_knowsTheUnlockedOnesByNumber()
    {
        QTemporaryDir user;
        const QString folder = user.filePath(QStringLiteral("trophy/") + Trophies::BLOODBORNE_NP_ID);
        writeText(folder + QStringLiteral("/unlocked.txt"), QStringLiteral("# the trophies unlocked here, one id a line\n2\n"));
        writeText(user.filePath(QStringLiteral("trophies.jsonl")), JOURNAL);
        const Trophies::Content content = Trophies::load(user.path(), QStringLiteral("1"));
        QCOMPARE(QFileInfo(content.folder).fileName(), Trophies::BLOODBORNE_NP_ID);
        QVERIFY(content.named == false);
        QCOMPARE(content.trophies.size(), UNLOCKED_TROPHIES);
        QCOMPARE(content.trophies.at(FIRST_TROPHY).id, SECRET_ID);
        QCOMPARE(content.trophies.at(SECOND_TROPHY).id, BRONZE_ID);
        QCOMPARE(content.unlocked, UNLOCKED_TROPHIES);
        // The first unlock's time.
        QCOMPARE(content.trophies.at(SECOND_TROPHY).time, QStringLiteral("2026-10-26T21:05:00Z"));
    }

    void load_withTheXml_namesEveryTrophyInTheGamesLanguage()
    {
        QTemporaryDir user;
        const QString folder = user.filePath(QStringLiteral("trophy/") + Trophies::BLOODBORNE_NP_ID);
        writeText(folder + QStringLiteral("/unlocked.txt"), QStringLiteral("2\n"));
        writeText(user.filePath(QStringLiteral("trophies.jsonl")), JOURNAL);
        writeText(folder + QStringLiteral("/TROP.XML"), XML);
        writeText(folder + QStringLiteral("/TROP_08.XML"), QString(XML).replace(QStringLiteral("All of it"), QStringLiteral("Everything, in Russian")));
        writeText(folder + QStringLiteral("/TROP000.PNG"), QStringLiteral("\x89PNG"));

        const Trophies::Content content = Trophies::load(user.path(), QStringLiteral("8"));
        QVERIFY(content.named);
        QCOMPARE(content.total, XML_TROPHIES);
        QCOMPARE(content.unlocked, UNLOCKED_TROPHIES);
        const Trophies::Trophy& first = content.trophies.at(FIRST_TROPHY);
        QCOMPARE(first.name, QStringLiteral("Everything, in Russian"));
        QCOMPARE(first.grade, Trophies::PLATINUM);
        QVERIFY(first.unlocked == false);
        QCOMPARE(QFileInfo(first.icon).fileName(), QStringLiteral("TROP000.PNG"));
        const Trophies::Trophy& secret = content.trophies.at(SECOND_TROPHY);
        QCOMPARE(secret.name, QStringLiteral("Fish & Chips"));
        QVERIFY(secret.hidden);
        QVERIFY(secret.icon.isEmpty());
        QCOMPARE(content.grades.value(Trophies::PLATINUM).unlocked, 0);
        QCOMPARE(content.grades.value(Trophies::PLATINUM).total, 1);
        QCOMPARE(content.grades.value(Trophies::GOLD).unlocked, 1);
        QCOMPARE(content.grades.value(Trophies::BRONZE).unlocked, 1);
        QVERIFY(content.grades.contains(Trophies::SILVER) == false);

        // Another language without its own file: TROP.XML.
        QCOMPARE(Trophies::load(user.path(), QStringLiteral("1")).trophies.at(FIRST_TROPHY).name, QStringLiteral("All of it"));
    }

    void parseXml_damagedFile_givesNone()
    {
        QVERIFY(Trophies::parseXml(QStringLiteral("<trophyconf><trophy id=\"0\"><name>x</trophy>")).isEmpty());
        QVERIFY(Trophies::parseXml(QStringLiteral("no xml here")).isEmpty());
        QCOMPARE(Trophies::parseXml(XML).size(), XML_TROPHIES);
    }

    void trophyFolder_otherTitles_takeTheNewest()
    {
        QTemporaryDir user;
        writeText(user.filePath(QStringLiteral("trophy/NPWR00000_00/unlocked.txt")), QStringLiteral("1\n"));
        QDir().mkpath(user.filePath(QStringLiteral("trophy/empty")));
        QCOMPARE(QFileInfo(Trophies::trophyFolder(user.path())).fileName(), QStringLiteral("NPWR00000_00"));
    }

    void unlockTime_utcStamp_isAValidLocalTime()
    {
        const QDateTime moment = Trophies::unlockTime(QStringLiteral("2026-10-26T21:05:00Z"));
        QVERIFY(moment.isValid());
        QCOMPARE(moment.toUTC().time(), QTime(21, 5));
        QVERIFY(Trophies::unlockTime(QStringLiteral("yesterday")).isValid() == false);
    }

    void validKey_needs32HexadecimalDigits()
    {
        QVERIFY(Trophies::validKey(QStringLiteral(" 00112233445566778899aabbccddEEFF ")));
        QVERIFY(Trophies::validKey(QStringLiteral("0011")) == false);
        QVERIFY(Trophies::validKey(QStringLiteral("zz").repeated(Trophies::KEY_DIGITS / 2)) == false);
    }
};

QTEST_MAIN(TstTrophies)
#include "tst_trophies.moc"
