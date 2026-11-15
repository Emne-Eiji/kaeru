// Kaeru unit tests — work log policies (spec §10, WP-05 storage core)
// Honest accounting rules: open segments never count as work, paused/locked
// segments are excluded from totals, sessions are single-active.
#include "app/Paths.h"
#include "storage/Database.h"
#include "storage/Migrations.h"
#include "storage/ProjectRepository.h"
#include "storage/SessionRepository.h"

#include <QDateTime>
#include <QTemporaryDir>
#include <QtTest>

using namespace kaeru;

class TestWorkLog final : public QObject {
    Q_OBJECT

private slots:
    void initTestCase()
    {
        // Isolated data home — tests never touch the user's real data.
        QVERIFY(m_home.isValid());
        Paths::applyTestSandbox(m_home.path());
    }

    void openSegmentNeverCountsAsWork()
    {
        Database db(m_home.path() + QStringLiteral("/wl1.sqlite"));
        QString err;
        QVERIFY2(db.open(&err), qPrintable(err));
        QVERIFY2(Migrations::apply(db.handle(), &err), qPrintable(err));
        const qint64 pid = ensureProject(db, err);
        QVERIFY2(pid > 0, qPrintable(err));
        SessionRepository repo(db.handle());

        QDateTime t = QDateTime(QDate(2026, 11, 15), QTime(9, 0), Qt::UTC);
        const qint64 sid = repo.startSession(pid, t, &err);
        QVERIFY2(sid > 0, qPrintable(err));
        QVERIFY(repo.appendSegment(sid, t, QStringLiteral("work"), QStringLiteral("manual"), &err) > 0);

        SessionRepository::Totals totals;
        QVERIFY(repo.totalsBetween(t.addSecs(-60), t.addSecs(3600), 0, &totals, &err));
        QCOMPARE(totals.workMs, qint64(0)); // segment still open → no work counted

        QVERIFY(repo.closeOpenSegment(sid, t.addSecs(1800), &err) > 0);
        QVERIFY(repo.totalsBetween(t.addSecs(-60), t.addSecs(3600), 0, &totals, &err));
        QCOMPARE(totals.workMs, qint64(1800 * 1000));
        QCOMPARE(totals.segmentCount, 1);
    }

    void pausedSegmentsExcludedFromWorkTotal()
    {
        Database db(m_home.path() + QStringLiteral("/wl2.sqlite"));
        QString err;
        QVERIFY2(db.open(&err), qPrintable(err));
        QVERIFY2(Migrations::apply(db.handle(), &err), qPrintable(err));
        const qint64 pid = ensureProject(db, err);
        QVERIFY2(pid > 0, qPrintable(err));
        SessionRepository repo(db.handle());

        QDateTime t = QDateTime(QDate(2026, 11, 15), QTime(10, 0), Qt::UTC);
        const qint64 sid = repo.startSession(pid, t, &err);
        QVERIFY2(sid > 0, qPrintable(err));
        QVERIFY(repo.appendSegment(sid, t, QStringLiteral("work"), QStringLiteral("manual"), &err) > 0);
        QVERIFY(repo.appendSegment(sid, t.addSecs(900), QStringLiteral("paused"), QStringLiteral("auto-lock"), &err) > 0);
        QVERIFY(repo.appendSegment(sid, t.addSecs(1200), QStringLiteral("work"), QStringLiteral("manual"), &err) > 0);
        QVERIFY(repo.closeActiveSession(t.addSecs(2400), nullptr, &err));

        SessionRepository::Totals totals;
        QVERIFY(repo.totalsBetween(t.addSecs(-60), t.addSecs(3600), 0, &totals, &err));
        QCOMPARE(totals.workMs, qint64(2100 * 1000));
        QCOMPARE(totals.breakMs, qint64(300 * 1000));
        QCOMPARE(totals.segmentCount, 3);
    }

    void appendingClosesPreviousSegmentNoOverlap()
    {
        Database db(m_home.path() + QStringLiteral("/wl3.sqlite"));
        QString err;
        QVERIFY2(db.open(&err), qPrintable(err));
        QVERIFY2(Migrations::apply(db.handle(), &err), qPrintable(err));
        const qint64 pid = ensureProject(db, err);
        QVERIFY2(pid > 0, qPrintable(err));
        SessionRepository repo(db.handle());

        QDateTime t = QDateTime(QDate(2026, 11, 15), QTime(11, 0), Qt::UTC);
        const qint64 sid = repo.startSession(pid, t, &err);
        QVERIFY2(sid > 0, qPrintable(err));
        QVERIFY(repo.appendSegment(sid, t, QStringLiteral("work"), QStringLiteral("manual"), &err) > 0);
        QVERIFY(repo.appendSegment(sid, t.addSecs(600), QStringLiteral("work"), QStringLiteral("manual"), &err) > 0);
        QVERIFY(repo.appendSegment(sid, t.addSecs(1500), QStringLiteral("work"), QStringLiteral("manual"), &err) > 0);

        const QList<SegmentRecord> segs = repo.segmentsForSession(sid, &err);
        QCOMPARE(segs.size(), 3);
        // No overlaps: each segment ends where the next begins.
        QCOMPARE(segs.at(0).endedUtc.isValid(), true);
        QCOMPARE(segs.at(0).endedUtc, segs.at(1).startedUtc);
        QCOMPARE(segs.at(1).endedUtc, segs.at(2).startedUtc);
        QCOMPARE(segs.at(2).endedUtc.isValid(), false); // last still open
    }

    void singleActiveSessionEnforced()
    {
        Database db(m_home.path() + QStringLiteral("/wl4.sqlite"));
        QString err;
        QVERIFY2(db.open(&err), qPrintable(err));
        QVERIFY2(Migrations::apply(db.handle(), &err), qPrintable(err));
        const qint64 pid = ensureProject(db, err);
        QVERIFY2(pid > 0, qPrintable(err));
        SessionRepository repo(db.handle());

        QDateTime t = QDateTime(QDate(2026, 11, 15), QTime(12, 0), Qt::UTC);
        const qint64 s1 = repo.startSession(pid, t, &err);
        QVERIFY2(s1 > 0, qPrintable(err));
        err.clear();
        const qint64 s2 = repo.startSession(pid, t.addSecs(10), &err);
        QCOMPARE(s2, qint64(0)); // second concurrent session refused
        QVERIFY(!err.isEmpty()); // honest error, never guessed
        QVERIFY(repo.closeActiveSession(t.addSecs(60), nullptr, &err));
        const qint64 s3 = repo.startSession(pid, t.addSecs(70), &err);
        QVERIFY2(s3 > 0, qPrintable(err)); // allowed again after closing
    }

    void closingSessionClosesItsSegments()
    {
        Database db(m_home.path() + QStringLiteral("/wl5.sqlite"));
        QString err;
        QVERIFY2(db.open(&err), qPrintable(err));
        QVERIFY2(Migrations::apply(db.handle(), &err), qPrintable(err));
        const qint64 pid = ensureProject(db, err);
        QVERIFY2(pid > 0, qPrintable(err));
        SessionRepository repo(db.handle());

        QDateTime t = QDateTime(QDate(2026, 11, 15), QTime(13, 0), Qt::UTC);
        const qint64 sid = repo.startSession(pid, t, &err);
        QVERIFY2(sid > 0, qPrintable(err));
        QVERIFY(repo.appendSegment(sid, t, QStringLiteral("work"), QStringLiteral("manual"), &err) > 0);
        QVERIFY(repo.closeSession(sid, t.addSecs(1200), &err));

        SessionRecord s;
        QVERIFY(repo.sessionById(sid, &s, &err));
        QCOMPARE(s.open, false);
        QCOMPARE(s.endedUtc.isValid(), true);
        const QList<SegmentRecord> segs = repo.segmentsForSession(sid, &err);
        QCOMPARE(segs.size(), 1);
        QCOMPARE(segs.first().endedUtc.isValid(), true);
        QCOMPARE(segs.first().endedUtc, t.addSecs(1200));
    }

    void checkpointsAreNotReportData()
    {
        // Recovery checkpoints exist only for resume evidence; totals read
        // segments, so checkpoints can never duplicate report entries.
        Database db(m_home.path() + QStringLiteral("/wl6.sqlite"));
        QString err;
        QVERIFY2(db.open(&err), qPrintable(err));
        QVERIFY2(Migrations::apply(db.handle(), &err), qPrintable(err));
        const qint64 pid = ensureProject(db, err);
        QVERIFY2(pid > 0, qPrintable(err));
        SessionRepository repo(db.handle());

        QDateTime t = QDateTime(QDate(2026, 11, 15), QTime(14, 0), Qt::UTC);
        const qint64 sid = repo.startSession(pid, t, &err);
        QVERIFY2(sid > 0, qPrintable(err));
        const qint64 seg = repo.appendSegment(sid, t, QStringLiteral("work"), QStringLiteral("manual"), &err);
        QVERIFY(seg > 0);
        QVERIFY(repo.writeCheckpoint(sid, seg, t.addSecs(600), QStringLiteral("60s"), &err));
        QVERIFY(repo.writeCheckpoint(sid, seg, t.addSecs(700), QStringLiteral("60s"), &err));

        SessionRepository::Totals totals;
        QVERIFY(repo.totalsBetween(t.addSecs(-60), t.addSecs(3600), 0, &totals, &err));
        QCOMPARE(totals.workMs, qint64(0)); // checkpoints added nothing to totals

        QVERIFY(repo.closeActiveSession(t.addSecs(1000), nullptr, &err));
        QVERIFY(repo.deleteCheckpointsForSession(sid, &err));
        QVERIFY(repo.totalsBetween(t.addSecs(-60), t.addSecs(3600), 0, &totals, &err));
        QCOMPARE(totals.workMs, qint64(1000 * 1000));
    }

    void focusSessionsRecordedSeparately()
    {
        Database db(m_home.path() + QStringLiteral("/wl7.sqlite"));
        QString err;
        QVERIFY2(db.open(&err), qPrintable(err));
        QVERIFY2(Migrations::apply(db.handle(), &err), qPrintable(err));
        SessionRepository repo(db.handle());

        QDateTime t = QDateTime(QDate(2026, 11, 15), QTime(15, 0), Qt::UTC);
        QVERIFY(repo.insertFocusSession(t, t.addSecs(1500), 25 * 60 * 1000, 25 * 60 * 1000,
                                        QStringLiteral("work"), QStringLiteral("pomodoro"), &err));
        // Focus data must not leak into work-log totals.
        SessionRepository::Totals totals;
        QVERIFY(repo.totalsBetween(t.addSecs(-60), t.addSecs(3600), 0, &totals, &err));
        QCOMPARE(totals.workMs, qint64(0));
        QCOMPARE(totals.segmentCount, 0);
    }

    void dailyTotalsRespectOffset()
    {
        Database db(m_home.path() + QStringLiteral("/wl8.sqlite"));
        QString err;
        QVERIFY2(db.open(&err), qPrintable(err));
        QVERIFY2(Migrations::apply(db.handle(), &err), qPrintable(err));
        const qint64 pid = ensureProject(db, err);
        QVERIFY2(pid > 0, qPrintable(err));
        SessionRepository repo(db.handle());

        // 22:30Z = 17:30 in Toronto (-5h in November) → same local day.
        QDateTime t = QDateTime(QDate(2026, 11, 15), QTime(22, 30), Qt::UTC);
        const qint64 sid = repo.startSession(pid, t, &err);
        QVERIFY2(sid > 0, qPrintable(err));
        QVERIFY(repo.appendSegment(sid, t, QStringLiteral("work"), QStringLiteral("manual"), &err) > 0);
        QVERIFY(repo.closeActiveSession(t.addSecs(1200), nullptr, &err));

        const QList<SessionRepository::DailyTotal> days =
            repo.dailyTotals(t.addSecs(-3600), t.addSecs(3600), -5 * 3600, &err);
        QCOMPARE(days.size(), 1);
        QCOMPARE(days.first().day, QDate(2026, 11, 15));
        QCOMPARE(days.first().workMs, qint64(1200 * 1000));
    }

private:
    // Fresh project row for a freshly migrated database.
    qint64 ensureProject(Database& db, QString& err)
    {
        ProjectRepository projects(db.handle());
        qint64 id = 0;
        if (!projects.ensureDefaultProject(QStringLiteral("Default"), &id, &err))
            return 0;
        return id;
    }

    QTemporaryDir m_home;
};

QTEST_MAIN(TestWorkLog)
#include "test_worklog.moc"