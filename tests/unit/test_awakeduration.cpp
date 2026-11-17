// Kaeru unit tests — shared Keep Awake duration model (WP-08)
#include "core/AwakeDuration.h"

#include <QtTest>

using namespace kaeru;

class TestAwakeDuration final : public QObject {
    Q_OBJECT

private slots:
    void standardDurations_data()
    {
        QTest::addColumn<int>("kind");
        QTest::addColumn<qint64>("ms");
        QTest::addColumn<QString>("token");

        QTest::newRow("15m") << int(AwakeDuration::Kind::Minutes15)
                             << qint64(15 * 60 * 1000) << QStringLiteral("15m");
        QTest::newRow("30m") << int(AwakeDuration::Kind::Minutes30)
                             << qint64(30 * 60 * 1000) << QStringLiteral("30m");
        QTest::newRow("1h") << int(AwakeDuration::Kind::Hour1)
                            << qint64(60 * 60 * 1000) << QStringLiteral("1h");
        QTest::newRow("2h") << int(AwakeDuration::Kind::Hours2)
                            << qint64(120 * 60 * 1000) << QStringLiteral("2h");
        QTest::newRow("indefinite") << int(AwakeDuration::Kind::Indefinite)
                                    << qint64(-1) << QStringLiteral("indefinite");
    }

    void standardDurations()
    {
        QFETCH(int, kind);
        QFETCH(qint64, ms);
        QFETCH(QString, token);

        const AwakeDuration d = AwakeDuration::standard(
            static_cast<AwakeDuration::Kind>(kind));
        QCOMPARE(d.durationMs(), ms);
        QCOMPARE(d.token(), token);

        AwakeDuration restored;
        QVERIFY(AwakeDuration::fromToken(token, &restored));
        QCOMPARE(restored.kind, d.kind);
        QCOMPARE(restored.durationMs(), ms);
    }

    void customDurationRoundTrip()
    {
        AwakeDuration d;
        QString error;
        QVERIFY(AwakeDuration::custom(1, 30, &d, &error));
        QVERIFY(error.isEmpty());
        QVERIFY(d.isCustom());
        QVERIFY(!d.isIndefinite());
        QCOMPARE(d.durationMs(), qint64(90 * 60 * 1000));
        QCOMPARE(d.token(), QStringLiteral("custom:5400000"));
        QCOMPARE(d.label(), QStringLiteral("Custom · 1h 30m"));

        AwakeDuration restored;
        QVERIFY(AwakeDuration::fromToken(d.token(), &restored));
        QCOMPARE(restored.customMs, d.customMs);
        QCOMPARE(restored.durationMs(), d.durationMs());
    }

    void customValidation_data()
    {
        QTest::addColumn<int>("hours");
        QTest::addColumn<int>("minutes");
        QTest::addColumn<bool>("valid");

        QTest::newRow("one-minute") << 0 << 1 << true;
        QTest::newRow("many-hours") << 48 << 0 << true;
        QTest::newRow("max") << 168 << 0 << true;
        QTest::newRow("zero") << 0 << 0 << false;
        QTest::newRow("negative-hours") << -1 << 30 << false;
        QTest::newRow("negative-minutes") << 1 << -1 << false;
        QTest::newRow("minutes-60") << 1 << 60 << false;
        QTest::newRow("overflow") << 168 << 1 << false;
    }

    void customValidation()
    {
        QFETCH(int, hours);
        QFETCH(int, minutes);
        QFETCH(bool, valid);

        AwakeDuration d;
        QString error;
        QCOMPARE(AwakeDuration::custom(hours, minutes, &d, &error), valid);
        QCOMPARE(error.isEmpty(), valid);
    }

    void corruptTokensRejected()
    {
        const QStringList bad {
            QString(), QStringLiteral("3h"), QStringLiteral("custom:0"),
            QStringLiteral("custom:-1"), QStringLiteral("custom:30000"),
            QStringLiteral("custom:90061000"), QStringLiteral("custom:1h"),
            QStringLiteral("INDEFINITE"),
        };
        for (const QString& token : bad) {
            AwakeDuration d;
            QVERIFY2(!AwakeDuration::fromToken(token, &d),
                     qPrintable(QStringLiteral("accepted bad token: %1").arg(token)));
        }
    }

    void customIsFiniteAndDistinctFromIndefinite()
    {
        AwakeDuration custom;
        QVERIFY(AwakeDuration::custom(168, 0, &custom));
        const AwakeDuration indefinite =
            AwakeDuration::standard(AwakeDuration::Kind::Indefinite);
        QVERIFY(custom.isCustom());
        QVERIFY(!custom.isIndefinite());
        QVERIFY(indefinite.isIndefinite());
        QVERIFY(custom.durationMs() > 0);
        QCOMPARE(indefinite.durationMs(), qint64(-1));
    }
};

QTEST_MAIN(TestAwakeDuration)
#include "test_awakeduration.moc"
