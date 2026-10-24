#include <QtTest/QtTest>

#include "profiles.h"

class TstProfiles : public QObject
{
    Q_OBJECT

private slots:
    void arrangeMods_savedOrder_comesFirstAndNewModsAreAppended()
    {
        const QJsonObject profile {
            { QStringLiteral("order"), QJsonArray { QStringLiteral("B"), QStringLiteral("Gone"), QStringLiteral("A") } },
            { QStringLiteral("disabled"), QJsonArray { QStringLiteral("A") } },
        };
        const QList<Profiles::Mod> mods = Profiles::arrangeMods({ QStringLiteral("A"), QStringLiteral("B"), QStringLiteral("C") }, profile);
        QCOMPARE(mods.size(), 3);
        QCOMPARE(mods.at(0).name, QStringLiteral("B"));
        QCOMPARE(mods.at(1).name, QStringLiteral("A"));
        QVERIFY(mods.at(1).enabled == false);
        QCOMPARE(mods.at(2).name, QStringLiteral("C"));
        QVERIFY(mods.at(2).enabled);
    }

    void modsJson_listsOrderAndDisabled()
    {
        const QJsonObject json = Profiles::modsJson({ { QStringLiteral("X"), true }, { QStringLiteral("Y"), false } });
        QCOMPARE(json.value(QStringLiteral("order")).toArray().size(), 2);
        QCOMPARE(json.value(QStringLiteral("disabled")).toArray().first().toString(), QStringLiteral("Y"));
    }

    void applyPatchProfile_defaultsAndChoices_decide()
    {
        QList<Profiles::Patch> patches = {
            { QStringLiteral("a.xml/On"), QStringLiteral("On"), {}, {}, true, false },
            { QStringLiteral("a.xml/Off"), QStringLiteral("Off"), {}, {}, false, false },
            { QStringLiteral("a.xml/Chosen"), QStringLiteral("Chosen"), {}, {}, false, false },
            { QStringLiteral("a.xml/Dropped"), QStringLiteral("Dropped"), {}, {}, true, false },
        };
        const QJsonObject profile {
            { QStringLiteral("enabled"), QJsonArray { QStringLiteral("a.xml/Chosen") } },
            { QStringLiteral("disabled"), QJsonArray { QStringLiteral("a.xml/Dropped") } },
        };
        Profiles::applyPatchProfile(patches, profile);
        QVERIFY(patches.at(0).enabled);
        QVERIFY(patches.at(1).enabled == false);
        QVERIFY(patches.at(2).enabled);
        QVERIFY(patches.at(3).enabled == false);
    }

    void patchesJson_onlyDifferencesAndOtherFoldersChoices_areKept()
    {
        const QList<Profiles::Patch> shown = {
            { QStringLiteral("a.xml/On"), {}, {}, {}, true, true },
            { QStringLiteral("a.xml/Off"), {}, {}, {}, true, false },
            { QStringLiteral("a.xml/New"), {}, {}, {}, false, true },
        };
        const QJsonObject profile {
            { QStringLiteral("enabled"), QJsonArray { QStringLiteral("other.xml/Kept"), QStringLiteral("a.xml/On") } },
        };
        const QJsonObject json = Profiles::patchesJson(shown, profile);
        QCOMPARE(json.value(QStringLiteral("enabled")).toArray(),
                 (QJsonArray { QStringLiteral("a.xml/New"), QStringLiteral("other.xml/Kept") }));
        QCOMPARE(json.value(QStringLiteral("disabled")).toArray(), QJsonArray { QStringLiteral("a.xml/Off") });
    }
};

QTEST_MAIN(TstProfiles)
#include "tst_profiles.moc"
