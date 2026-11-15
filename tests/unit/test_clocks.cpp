// Kaeru unit tests — clocks, durations, wall time (ADR-003)
// Deterministic via FakeTimeSource; no real suspend is ever triggered.
#include "core/Clock.h"
#include "core/Duration.h"

#include <QtTest>

using namespace kaeru;

class TestClocks final : public QObject {
    Q_OBJECT

private slots:
    void fakeTimeSourceAdvances()
    {
        FakeTimeSource t(1000);
        QCOMPARE(t.nowMs(), qint64(1000));
        t.advanceMs(500);
        QCOMPARE(t.nowMs(), qint64(1500));
        t.setNowMs(42);
        QCOMPARE(t.nowMs(), qint64(42));
        QCOMPARE(t.kind(), ClockKind::Monotonic);
    }

    void monotonicIsNonDecreasing()
    {
        MonotonicTimeSource t;
        const qint64 a = t.nowMs();
        QCOMPARE(t.kind(), ClockKind::Monotonic);
        const qint64 b = t.nowMs();
        QVERIFY(b >= a);
    }

    void bootClockAvailableOnLinux()
    {
        // CLOCK_BOOTTIME is expected on Linux kernels (honest fallback if not).
        QCOMPARE(BootTimeSource::available(), true);
        BootTimeSource t;
        QCOMPARE(t.kind(), ClockKind::Boot);
        const qint64 a = t.nowMs();
        const qint64 b = t.nowMs();
        QVERIFY(b >= a);
    }

    void wallSourceUtcAndZones()
    {
        WallTimeSource utc(QStringLiteral("UTC"));
        QVERIFY(utc.timeZone().isValid());
        QCOMPARE(utc.nowUtc().timeSpec(), Qt::UTC);

        WallTimeSource toronto(QStringLiteral("America/Toronto"));
        QVERIFY(toronto.timeZone().isValid());
        const QDateTime local = toronto.nowLocal();
        QCOMPARE(local.timeZone(), toronto.timeZone());
        // Local conversion must agree with UTC instant.
        QCOMPARE(local.toUTC().msecsTo(toronto.nowUtc()), qint64(0));
    }

    void parseDurationValid()
    {
        qint64 ms = 0;
        QVERIFY(parseDurationMs(QStringLiteral("90s"), &ms));
        QCOMPARE(ms, qint64(90000));
        QVERIFY(parseDurationMs(QStringLiteral("30m"), &ms));
        QCOMPARE(ms, qint64(1800000));
        QVERIFY(parseDurationMs(QStringLiteral("2h"), &ms));
        QCOMPARE(ms, qint64(7200000));
        QVERIFY(parseDurationMs(QStringLiteral("1h30m"), &ms));
        QCOMPARE(ms, qint64(5400000));
        QVERIFY(parseDurationMs(QStringLiteral("25"), &ms));
        QCOMPARE(ms, qint64(25000));
        QVERIFY(parseDurationMs(QStringLiteral(" 10 m "), &ms));
        QCOMPARE(ms, qint64(600000));
    }

    void parseDurationRejectsMalformed()
    {
        qint64 ms = 0;
        QVERIFY(!parseDurationMs(QString(), &ms));
        QVERIFY(!parseDurationMs(QStringLiteral("0"), &ms));
        QVERIFY(!parseDurationMs(QStringLiteral("-5"), &ms));
        QVERIFY(!parseDurationMs(QStringLiteral("1x"), &ms));
        QVERIFY(!parseDurationMs(QStringLiteral("hm"), &ms));
        QVERIFY(!parseDurationMs(QStringLiteral("30x"), &ms));
        QVERIFY(!parseDurationMs(QStringLiteral("99999999999999"), &ms));
        QVERIFY(!parseDurationMs(QStringLiteral("2h30"), &ms)); // trailing digits
    }

    void parseIsoDateFormat()
    {
        QDate d;
        QVERIFY(parseIsoDate(QStringLiteral("2026-11-15"), &d));
        QCOMPARE(d, QDate(2026, 11, 15));
        QVERIFY(!parseIsoDate(QStringLiteral("2026-13-01"), &d));
        QVERIFY(!parseIsoDate(QStringLiteral("15/11/2026"), &d));
        QVERIFY(!parseIsoDate(QString(), &d));
        QVERIFY(!parseIsoDate(QStringLiteral("2026-1-1"), &d));
    }

    void formatDurations()
    {
        QCOMPARE(formatDurationMs(0), QStringLiteral("0m 00s"));
        QCOMPARE(formatDurationMs(3725000), QStringLiteral("1h 02m 05s"));
        QCOMPARE(formatDurationMs(90000), QStringLiteral("1m 30s"));
        QCOMPARE(formatDurationMs(-5), QStringLiteral("0m 00s"));
        QCOMPARE(formatRemainingMs(18 * 60 * 1000 + 5000), QStringLiteral("18m 05s"));
        QCOMPARE(formatRemainingMs(45 * 1000 + 10), QStringLiteral("46s"));
        QCOMPARE(formatRemainingMs(2 * 3600 * 1000 + 15 * 60 * 1000), QStringLiteral("2h 15m"));
        QCOMPARE(formatRemainingMs(-1), QStringLiteral("—"));
    }
};

QTEST_MAIN(TestClocks)
#include "test_clocks.moc"
