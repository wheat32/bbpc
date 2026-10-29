#include <QtTest/QtTest>

#include "choices.h"

namespace
{
QStringList values(const QList<Choices::Choice>& choices)
{
    QStringList list;
    for (const Choices::Choice& choice : choices)
    {
        list.append(QString::fromLatin1(choice.value));
    }
    return list;
}
} // namespace

class TstChoices : public QObject
{
    Q_OBJECT

private slots:
    void addBinding_newKey_isAppended()
    {
        QCOMPARE(Choices::addBinding(QStringLiteral("E, Mouse X2"), QStringLiteral("Q")), QStringLiteral("E, Mouse X2, Q"));
    }

    void addBinding_keyAlreadyBound_isKeptOnce()
    {
        QCOMPARE(Choices::addBinding(QStringLiteral("E,Mouse X2"), QStringLiteral("E")), QStringLiteral("E, Mouse X2"));
    }

    void addBinding_unassigned_givesTheKeyAlone()
    {
        QCOMPARE(Choices::addBinding(QString(), QStringLiteral("Left Alt")), QStringLiteral("Left Alt"));
    }

    void addBinding_fifthKey_dropsTheOldest()
    {
        const QString four = QStringLiteral("1, 2, 3, 4");
        const QString bound = Choices::addBinding(four, QStringLiteral("5"));
        QCOMPARE(bound, QStringLiteral("2, 3, 4, 5"));
        QCOMPARE(bound.split(u',').size(), Choices::MAX_KEYS_PER_ACTION);
    }

    void effects_gameAntiAliasing_isSetByTheAntiAliasingChoice()
    {
        for (const Choices::Effect& effect : Choices::effects())
        {
            QVERIFY(QString::fromLatin1(effect.key) != QStringLiteral("effect_game_aa"));
        }
        QVERIFY(values(Choices::antiAliasing()).contains(QStringLiteral("smaa")));
    }

    void controls_slowWalk_isAKeyboardBindingOnly()
    {
        bool found = false;
        for (const Choices::Control& control : Choices::controls())
        {
            if (QString::fromLatin1(control.name) != QStringLiteral("walk")) continue;
            found = true;
            QCOMPARE(QString::fromLatin1(control.keyDefault), QStringLiteral("Left Alt"));
            QVERIFY(control.padDefault == nullptr);
        }
        QVERIFY(found);
    }

    void upscalers_fsr1_isOffered()
    {
        QVERIFY(values(Choices::upscalers()).contains(QStringLiteral("fsr1")));
        QCOMPARE(Choices::indexOf(Choices::upscalers(), QStringLiteral("fsr4")), 0);
    }

    void voiceLanguages_areTheOnesWithVoiceFiles()
    {
        const QStringList voices = values(Choices::gameVoiceLanguages());
        QCOMPARE(voices.first(), QStringLiteral("auto"));
        QVERIFY(voices.contains(QStringLiteral("ptbr")));
        QVERIFY(voices.contains(QStringLiteral("ruru")) == false);
        QVERIFY(values(Choices::gameTextLanguages()).contains(QStringLiteral("ruru")));
    }
};

QTEST_MAIN(TstChoices)
#include "tst_choices.moc"
