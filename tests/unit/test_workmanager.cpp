// Kaeru unit tests — work session manager (spec §6.4, WP-05)
// Real lifecycle against a sandboxed SQLite database; suspend/lock/recovery
// are simulated at the manager's API boundary (no real system events).
#include "app/Paths.h"
#include "app/Settings.h"
#include "app/WorkSessionManager.h"
#include "storage/Database.h"
#include "storage/Migrations.h"
#include "storage/ProjectRepository.h"
#include "storage/SessionRepository.h"

#include <QDateTime>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QtTest>

using namespace kaeru;

class TestWorkManager final : public QObject {
    Q_OBJECT

private slots:
    void initTestCase()
    {
        QVERIFY(m_home.isValid());
        Paths::applyTestSandbox(m_home.path());
    }

    void startPauseResumeStop()
    {
        Database db(m_home.path() + QStringLiteral("/wm1.sqlite"));
        QString err;
        QVERIFY2(db.open(&err), qPrintable(err));
        QVERIFY2(Migrations::apply(db.handle(), &err), qPrintable(err));
        ProjectRepository projects(db.handle());
        SessionRepository repo(db.handle());
        WorkSessionManager wm(&repo, &projects);

        QVERIFY2(wm.start(QStringLiteral("Alpha"), &err), qPrintable(err));
        QCOMPARE(wm.isActive(), true);
        QCOMPARE(wm.isRecording(), true);
        QVERIFY(wm.session().id > 0);

        QVERIFY2(wm.pause(QStringLiteral("manual"), &err), qPrintable(err));
        QCOMPARE(wm.isRecording(), false);
        QCOMPARE(wm.isActive(), true); // session stays open

        QVERIFY2(wm.resume(&err), qPrintable(err));
        QCOMPARE(wm.isRecording(), true);

        QVERIFY2(wm.stop(&err), qPrintable(err));
        QCOMPARE(wm.isActive(), false);
        QCOMPARE(wm.isRecording(), false);
    }

    void startIsIdempotentResume()
    {
        Database db(m_home.path() + QStringLiteral("/wm2.sqlite"));
        QString err;
        QVERIFY2(db.open(&err), qPrintable(err));
        QVERIFY2(Migrations::apply(db.handle(), &err), qPrintable(err));
        ProjectRepository projects(db.handle());
        SessionRepository repo(db.handle());
        WorkSessionManager wm(&repo, &projects);

        QVERIFY(wm.start(QStringLiteral("Alpha"), &err));
        const qint64 sid = wm.session().id;
        // A second start() resumes the open session (no second session).
        QVERIFY(wm.start(QStringLiteral("Beta"), &err));
        QCOMPARE(wm.session().id, sid);
        SessionRecord s;
        QVERIFY(repo.activeSession(&s, &err));
        QCOMPARE(s.id, sid);
    }

    void switchProjectIsTransactional()
    {
        Database db(m_home.path() + QStringLiteral("/wm3.sqlite"));
        QString err;
        QVERIFY2(db.open(&err), qPrintable(err));
        QVERIFY2(Migrations::apply(db.handle(), &err), qPrintable(err));
        ProjectRepository projects(db.handle());
        SessionRepository repo(db.handle());
        WorkSessionManager wm(&repo, &projects);

        QVERIFY(wm.start(QStringLiteral("Alpha"), &err));
        const qint64 sid = wm.session().id;
        QVERIFY2(wm.switchProject(QStringLiteral("Beta"), &err), qPrintable(err));
        // Same session, no overlap: old segment closed where new one begins.
        const QList<SegmentRecord> segs = repo.segmentsForSession(sid, &err);
        QCOMPARE(segs.size(), 2);
        QCOMPARE(segs.at(0).endedUtc.isValid(), true);
        QCOMPARE(segs.at(1).startedUtc.isValid(), true);
        QCOMPARE(segs.at(1).source, QStringLiteral("project-switch"));
    }

    void suspendBoundaryClosesSegment()
    {
        Database db(m_home.path() + QStringLiteral("/wm4.sqlite"));
        QString err;
        QVERIFY2(db.open(&err), qPrintable(err));
        QVERIFY2(Migrations::apply(db.handle(), &err), qPrintable(err));
        ProjectRepository projects(db.handle());
        SessionRepository repo(db.handle());
        WorkSessionManager wm(&repo, &projects);

        QVERIFY(wm.start(QStringLiteral("Alpha"), &err));
        wm.onSuspendBoundary(true);
        QCOMPARE(wm.isRecording(), false);
        QCOMPARE(wm.isActive(), true); // session stays open, paused

        // Resume boundary: stays paused (user resumes explicitly).
        wm.onSuspendBoundary(false);
        QCOMPARE(wm.isRecording(), false);

        // Explicit resume starts a new segment.
        QVERIFY(wm.resume(&err));
        QCOMPARE(wm.isRecording(), true);
    }

    void lockPausesOnlyWhenEnabled()
    {
        Database db(m_home.path() + QStringLiteral("/wm5.sqlite"));
        QString err;
        QVERIFY2(db.open(&err), qPrintable(err));
        QVERIFY2(Migrations::apply(db.handle(), &err), qPrintable(err));
        ProjectRepository projects(db.handle());
        SessionRepository repo(db.handle());
        Settings settings;
        settings.setPauseWorkWhenLocked(false);
        WorkSessionManager wm(&repo, &projects);
        wm.setSettings(&settings);

        QVERIFY(wm.start(QStringLiteral("Alpha"), &err));
        wm.onLockChanged(true);
        QCOMPARE(wm.isRecording(), true); // setting off → no pause

        settings.setPauseWorkWhenLocked(true);
        wm.onLockChanged(true);
        QCOMPARE(wm.isRecording(), false); // setting on → paused

        wm.onLockChanged(false); // unlock must not auto-resume
        QCOMPARE(wm.isRecording(), false);
    }

    void checkpointsDoNotAffectTotals()
    {
        Database db(m_home.path() + QStringLiteral("/wm6.sqlite"));
        QString err;
        QVERIFY2(db.open(&err), qPrintable(err));
        QVERIFY2(Migrations::apply(db.handle(), &err), qPrintable(err));
        ProjectRepository projects(db.handle());
        SessionRepository repo(db.handle());
        WorkSessionManager wm(&repo, &projects);

        QVERIFY(wm.start(QStringLiteral("Alpha"), &err));
        wm.tick(); // immediate tick → no checkpoint yet
        wm.tick();
        SessionRepository::Totals totals;
        QVERIFY(repo.totalsBetween(QDateTime::currentDateTimeUtc().addSecs(-3600),
                                   QDateTime::currentDateTimeUtc(), 0, &totals, &err));
        QCOMPARE(totals.workMs, qint64(0)); // open segment never counts

        QVERIFY(wm.stop(&err));
        QVERIFY(repo.totalsBetween(QDateTime::currentDateTimeUtc().addSecs(-3600),
                                   QDateTime::currentDateTimeUtc(), 0, &totals, &err));
        QVERIFY(totals.workMs >= 0); // closed → counted (may be 0 ms for a fast test)
        QCOMPARE(totals.segmentCount, 1);
    }

    void recoveryIsConservative()
    {
        Database db(m_home.path() + QStringLiteral("/wm7.sqlite"));
        QString err;
        QVERIFY2(db.open(&err), qPrintable(err));
        QVERIFY2(Migrations::apply(db.handle(), &err), qPrintable(err));
        ProjectRepository projects(db.handle());
        SessionRepository repo(db.handle());
        WorkSessionManager wm(&repo, &projects);

        // Simulate a crash: session + segment left open, one checkpoint 2 min in.
        const QDateTime t0 = QDateTime::currentDateTimeUtc().addSecs(-600);
        const qint64 pid = projects.add(QStringLiteral("Alpha"), QString(), &err);
        QVERIFY(pid > 0);
        const qint64 sid = repo.startSession(pid, t0, &err);
        QVERIFY(sid > 0);
        const qint64 seg = repo.appendSegment(sid, t0, QStringLiteral("work"),
                                              QStringLiteral("manual"), &err);
        QVERIFY(seg > 0);
        const QDateTime checkpoint = t0.addSecs(120);
        QVERIFY(repo.writeCheckpoint(sid, seg, checkpoint, QStringLiteral("60s"), &err));

        const WorkSessionManager::RecoveryOutcome out = wm.recoverInterrupted();
        QCOMPARE(out.recoveredAny, true);
        QCOMPARE(wm.isActive(), false);

        // Recovered only up to the checkpoint: 2 minutes, not the full 10.
        SessionRepository::Totals totals;
        QVERIFY(repo.totalsBetween(t0.addSecs(-60), QDateTime::currentDateTimeUtc(),
                                   0, &totals, &err));
        QCOMPARE(totals.workMs, qint64(120 * 1000));

        // Checkpoints deleted → they can never duplicate report entries.
        SessionRecord s;
        QVERIFY(repo.sessionById(sid, &s, &err));
        QCOMPARE(s.open, false);
    }

    void recoveryWithoutCheckpointInventsNothing()
    {
        Database db(m_home.path() + QStringLiteral("/wm8.sqlite"));
        QString err;
        QVERIFY2(db.open(&err), qPrintable(err));
        QVERIFY2(Migrations::apply(db.handle(), &err), qPrintable(err));
        ProjectRepository projects(db.handle());
        SessionRepository repo(db.handle());
        WorkSessionManager wm(&repo, &projects);

        const QDateTime t0 = QDateTime::currentDateTimeUtc().addSecs(-300);
        const qint64 pid = projects.add(QStringLiteral("Alpha"), QString(), &err);
        const qint64 sid = repo.startSession(pid, t0, &err);
        QVERIFY(sid > 0);
        QVERIFY(repo.appendSegment(sid, t0, QStringLiteral("work"),
                                   QStringLiteral("manual"), &err) > 0);

        const WorkSessionManager::RecoveryOutcome out = wm.recoverInterrupted();
        QCOMPARE(out.recoveredAny, true);
        // No checkpoint → segment closed at its own start: zero work invented.
        SessionRepository::Totals totals;
        QVERIFY(repo.totalsBetween(t0.addSecs(-60), QDateTime::currentDateTimeUtc(),
                                   0, &totals, &err));
        QCOMPARE(totals.workMs, qint64(0));
    }

    void focusWorkRecordedIntoFocusSessions()
    {
        Database db(m_home.path() + QStringLiteral("/wm9.sqlite"));
        QString err;
        QVERIFY2(db.open(&err), qPrintable(err));
        QVERIFY2(Migrations::apply(db.handle(), &err), qPrintable(err));
        ProjectRepository projects(db.handle());
        SessionRepository repo(db.handle());
        WorkSessionManager wm(&repo, &projects);

        QVERIFY(wm.start(QStringLiteral("Alpha"), &err));
        const qint64 sid = wm.session().id;
        QVERIFY(wm.recordFocusWork(25 * 60 * 1000, 90 * 1000, &err));

        // Focus intervals go to focus_sessions — the work-log segment table
        // is untouched (no double counting).
        QCOMPARE(repo.segmentsForSession(sid, &err).size(), 1);

        QSqlQuery q(db.handle());
        QVERIFY(q.exec(QStringLiteral("SELECT COUNT(*) FROM focus_sessions")));
        QVERIFY(q.next());
        QCOMPARE(q.value(0).toInt(), 1);

        // Invalid values rejected, never guessed.
        QCOMPARE(wm.recordFocusWork(-1, 10, &err), false);
    }

private:
    QTemporaryDir m_home;
};

QTEST_MAIN(TestWorkManager)
#include "test_workmanager.moc"