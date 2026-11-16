// Kaeru unit tests — reports data (totals per project, focus totals, filters)
#include "app/Paths.h"
#include "storage/Database.h"
#include "storage/Migrations.h"
#include "storage/ProjectRepository.h"
#include "storage/SessionRepository.h"

#include <QDateTime>
#include <QTemporaryDir>
#include <QtTest>

using namespace kaeru;

class TestReports final : public QObject {
    Q_OBJECT

private slots:
    void initTestCase()
    {
        QVERIFY(m_home.isValid());
        Paths::applyTestSandbox(m_home.path());
    }

    void totalsByProjectAndFocus()
    {
        Database db(m_home.path() + QStringLiteral("/rep1.sqlite"));
        QString err;
        QVERIFY2(db.open(&err), qPrintable(err));
        QVERIFY2(Migrations::apply(db.handle(), &err), qPrintable(err));
        ProjectRepository projects(db.handle());
        SessionRepository repo(db.handle());

        const QDateTime t0 = QDateTime(QDate(2026, 11, 15), QTime(9, 0), Qt::UTC);
        const qint64 alpha = projects.add(QStringLiteral("Alpha"), QString(), &err);
        const qint64 beta = projects.add(QStringLiteral("Beta"), QString(), &err);
        QVERIFY(alpha > 0 && beta > 0);

        // Alpha: 10 min work + 2 min paused (closed).
        const qint64 s1 = repo.startSession(alpha, t0, &err);
        QVERIFY(s1 > 0);
        QVERIFY(repo.appendSegment(s1, t0, QStringLiteral("work"), QStringLiteral("manual"), &err) > 0);
        QVERIFY(repo.appendSegment(s1, t0.addSecs(600), QStringLiteral("paused"), QStringLiteral("manual"), &err) > 0);
        QVERIFY(repo.closeActiveSession(t0.addSecs(720), nullptr, &err));

        // Beta: 5 min work (closed).
        const qint64 s2 = repo.startSession(beta, t0.addSecs(1800), &err);
        QVERIFY(s2 > 0);
        QVERIFY(repo.appendSegment(s2, t0.addSecs(1800), QStringLiteral("work"), QStringLiteral("manual"), &err) > 0);
        QVERIFY(repo.closeActiveSession(t0.addSecs(2100), nullptr, &err));

        const QDateTime from = t0.addSecs(-60);
        const QDateTime to = t0.addSecs(4000);
        const QList<SessionRepository::ProjectTotal> byProject =
            repo.totalsByProject(from, to, &err);
        QVERIFY2(err.isEmpty(), qPrintable(err));
        QCOMPARE(byProject.size(), 2);
        QCOMPARE(byProject.at(0).name, QStringLiteral("Alpha"));
        QCOMPARE(byProject.at(0).workMs, qint64(600 * 1000));
        QCOMPARE(byProject.at(0).breakMs, qint64(120 * 1000));
        QCOMPARE(byProject.at(0).sessionCount, 1);
        QCOMPARE(byProject.at(1).name, QStringLiteral("Beta"));
        QCOMPARE(byProject.at(1).workMs, qint64(300 * 1000));
        QCOMPARE(byProject.at(1).breakMs, qint64(0));

        // Focus totals (closed focus_sessions only).
        QVERIFY(repo.insertFocusSession(t0, t0.addSecs(1500), 25 * 60 * 1000, 25 * 60 * 1000,
                                        QStringLiteral("work"), QStringLiteral("pomodoro"), &err));
        const SessionRepository::FocusTotal focus = repo.focusTotals(from, to, &err);
        QCOMPARE(focus.count, 1);
        QCOMPARE(focus.achievedMs, qint64(25 * 60 * 1000));

        // Out-of-range queries see nothing.
        const SessionRepository::FocusTotal none =
            repo.focusTotals(t0.addSecs(10000), t0.addSecs(20000), &err);
        QCOMPARE(none.count, 0);
        QCOMPARE(none.achievedMs, qint64(0));
    }

    void openSegmentsNeverCount()
    {
        Database db(m_home.path() + QStringLiteral("/rep2.sqlite"));
        QString err;
        QVERIFY2(db.open(&err), qPrintable(err));
        QVERIFY2(Migrations::apply(db.handle(), &err), qPrintable(err));
        ProjectRepository projects(db.handle());
        SessionRepository repo(db.handle());

        const QDateTime t0 = QDateTime(QDate(2026, 11, 15), QTime(10, 0), Qt::UTC);
        const qint64 pid = projects.add(QStringLiteral("Gamma"), QString(), &err);
        const qint64 sid = repo.startSession(pid, t0, &err);
        QVERIFY(sid > 0);
        QVERIFY(repo.appendSegment(sid, t0, QStringLiteral("work"), QStringLiteral("manual"), &err) > 0);
        // Segment still open → no totals.
        const QList<SessionRepository::ProjectTotal> byProject =
            repo.totalsByProject(t0.addSecs(-60), t0.addSecs(3600), &err);
        QCOMPARE(byProject.size(), 0);
    }

private:
    QTemporaryDir m_home;
};

QTEST_MAIN(TestReports)
#include "test_reports.moc"