#include <QtTest/QtTest>

#include "outputSizes.h"

class TstOutputSizes : public QObject
{
    Q_OBJECT

private slots:
    void resolvedAspect_unknownMonitor_is16by9()
    {
        QCOMPARE(OutputSizes::resolvedAspect(OutputSizes::AUTO, std::nullopt), QStringLiteral("16:9"));
    }

    void resolvedAspect_monitors_pickTheNearestGroup()
    {
        QCOMPARE(OutputSizes::resolvedAspect(OutputSizes::AUTO, QSize(1280, 800)), QStringLiteral("16:10"));
        QCOMPARE(OutputSizes::resolvedAspect(OutputSizes::AUTO, QSize(3440, 1440)), QStringLiteral("21:9"));
        QCOMPARE(OutputSizes::resolvedAspect(OutputSizes::AUTO, QSize(5120, 1440)), QStringLiteral("32:9"));
        QCOMPARE(OutputSizes::resolvedAspect(QStringLiteral("4:3"), QSize(1920, 1080)), QStringLiteral("4:3"));
    }

    void sizes_autoWithAnUnlistedMonitor_addsItInHeightOrder()
    {
        const QList<QSize> sizes = OutputSizes::sizes(OutputSizes::AUTO, QSize(2560, 1600));
        QVERIFY(sizes.contains(QSize(2560, 1600)));
        for (qsizetype i = 1; i < sizes.size(); ++i)
        {
            QVERIFY(sizes.at(i - 1).height() <= sizes.at(i).height());
        }
        // Another ratio's list does not get the monitor.
        QVERIFY(OutputSizes::sizes(QStringLiteral("16:9"), QSize(2560, 1600)).contains(QSize(2560, 1600)) == false);
    }

    void nearest_unlistedSize_keepsTheHeight()
    {
        const QList<QSize> sizes = OutputSizes::sizes(QStringLiteral("21:9"), std::nullopt);
        QCOMPARE(OutputSizes::nearest(sizes, QSize(1920, 1080)), QSize(2560, 1080));
        QCOMPARE(OutputSizes::nearest(sizes, QSize(3440, 1440)), QSize(3440, 1440));
    }

    void effective_autoOutput_isTheMonitorsResolution()
    {
        QCOMPARE(OutputSizes::effective(OutputSizes::AUTO, QSize(2560, 1440)), QSize(2560, 1440));
        QCOMPARE(OutputSizes::effective(OutputSizes::AUTO, std::nullopt), OutputSizes::DEFAULT_SIZE);
        QCOMPARE(OutputSizes::effective(QStringLiteral("1280x800"), QSize(2560, 1440)), OutputSizes::STEAM_DECK_SIZE);
        QCOMPARE(OutputSizes::effective(QStringLiteral("big"), QSize(2560, 1440)), OutputSizes::DEFAULT_SIZE);
    }

    void parse_textAndBack_roundTrips()
    {
        QCOMPARE(OutputSizes::parse(QStringLiteral("1280X720")), std::optional<QSize>(QSize(1280, 720)));
        QVERIFY(OutputSizes::parse(QStringLiteral("wide")).has_value() == false);
        QVERIFY(OutputSizes::parse(QStringLiteral("0x720")).has_value() == false);
        QCOMPARE(OutputSizes::format(QSize(3840, 2160)), QStringLiteral("3840x2160"));
    }
};

QTEST_MAIN(TstOutputSizes)
#include "tst_outputSizes.moc"
