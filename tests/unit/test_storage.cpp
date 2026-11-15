// Kaeru unit tests — storage layer: database, migrations, projects, sessions
#include "app/Paths.h"
#include "storage/Database.h"
#include "storage/Migrations.h"
#include "storage/ProjectRepository.h"
#include "storage/SessionRepository.h"

#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

using namespace kaeru;

class TestStorage final : public QObject {
    Q_OBJECT

private slots:
    void initTestCase()
    {
        QVERIFY(m_home.isValid());
        Paths::applyTestSandbox(m_home.path());
    }

    void databaseOpensAndIsIntegrityOk()
    {
        const QString file = m_home.path() + QStringLiteral("/a.sqlite");
        {
            Database db(file);
            QString err;
            QVERIFY2(db.open(&err), qPrintable(err));
        QVERIFY2(Migrations::apply(db.handle(), &err), qPrintable(err));
            QVERIFY(db.isOpen());
            QCOMPARE(db.filePath(), file);
            QString detail;
            QVERIFY(db.integrityOk(&detail));
            QCOMPARE(detail, QStringLiteral("ok"));
        }
        // Reopen works (WAL journal files tolerated).
        Database db2(file);
        QVERIFY(db2.open());
        QVERIFY(db2.integrityOk());
    }

    void migrationsAreVersionedAndIdempotent()
    {
        Database db(m_home.path() + QStringLiteral("/b.sqlite"));
        QString err;
        QVERIFY2(db.open(&err), qPrintable(err));
        QCOMPARE(Migrations::currentVersion(db.handle()), 0);
        QVERIFY2(Migrations::apply(db.handle(), &err), qPrintable(err));
        QCOMPARE(Migrations::currentVersion(db.handle()), Migrations::kLatestVersion);
        // Re-run is a no-op.
        QVERIFY2(Migrations::apply(db.handle(), &err), qPrintable(err));
        QCOMPARE(Migrations::currentVersion(db.handle()), Migrations::kLatestVersion);
        QVERIFY(db.integrityOk());
    }

    void projectCrudAndGuards()
    {
        Database db(m_home.path() + QStringLiteral("/c.sqlite"));
        QString err;
        QVERIFY2(db.open(&err), qPrintable(err));
        QVERIFY2(Migrations::apply(db.handle(), &err), qPrintable(err));
        ProjectRepository projects(db.handle());

        qint64 defaultId = 0;
        QVERIFY(projects.ensureDefaultProject(QStringLiteral("No project"), &defaultId, &err));
        QVERIFY(defaultId > 0);
        // Idempotent default creation.
        qint64 again = 0;
        QVERIFY(projects.ensureDefaultProject(QStringLiteral("No project"), &again, &err));
        QCOMPARE(again, defaultId);

        const qint64 webId = projects.add(QStringLiteral("Web site"), QStringLiteral("#3b82f6"), &err);
        QVERIFY(webId > 0);
        QCOMPARE(projects.add(QStringLiteral("  "), QString(), &err), qint64(0)); // empty refused

        ProjectRecord p;
        QVERIFY(projects.byId(webId, &p, &err));
        QCOMPARE(p.name, QStringLiteral("Web site"));
        QVERIFY(projects.byName(QStringLiteral("WEB SITE"), &p, &err)); // case-insensitive
        QCOMPARE(p.id, webId);

        QVERIFY(projects.rename(webId, QStringLiteral("Web"), &err));
        QVERIFY(!projects.rename(webId, QStringLiteral("   "), &err));

        // Projects without sessions may be deleted.
        QVERIFY(projects.remove(webId, &err));
        QVERIFY(projects.remove(defaultId, &err));
    }

    void projectWithSessionsIsNotDeletable()
    {
        Database db(m_home.path() + QStringLiteral("/d.sqlite"));
        QString err;
        QVERIFY2(db.open(&err), qPrintable(err));
        QVERIFY2(Migrations::apply(db.handle(), &err), qPrintable(err));
        ProjectRepository projects(db.handle());
        SessionRepository sessions(db.handle());

        const qint64 pid = projects.add(QStringLiteral("Keep me"), QString(), &err);
        QVERIFY(pid > 0);
        const QDateTime t = QDateTime(QDate(2026, 11, 15), QTime(9, 0), Qt::UTC);
        const qint64 sid = sessions.startSession(pid, t, &err);
        QVERIFY(sid > 0);
        QVERIFY(sessions.closeSession(sid, t.addSecs(60), &err));

        err.clear();
        QCOMPARE(projects.remove(pid, &err), false);
        QVERIFY(err.contains(QStringLiteral("archive")));
        QVERIFY(projects.setArchived(pid, true, &err));
    }

    void sessionLifecycleRoundTrip()
    {
        Database db(m_home.path() + QStringLiteral("/e.sqlite"));
        QString err;
        QVERIFY2(db.open(&err), qPrintable(err));
        QVERIFY2(Migrations::apply(db.handle(), &err), qPrintable(err));
        ProjectRepository projects(db.handle());
        SessionRepository sessions(db.handle());

        qint64 pid = 0;
        QVERIFY(projects.ensureDefaultProject(QStringLiteral("Default"), &pid, &err));

        const QDateTime t = QDateTime(QDate(2026, 11, 15), QTime(8, 30), Qt::UTC);
        const qint64 sid = sessions.startSession(pid, t, &err);
        QVERIFY(sid > 0);

        SessionRecord open;
        QVERIFY(sessions.activeSession(&open, &err));
        QCOMPARE(open.id, sid);
        QCOMPARE(open.open, true);

        QVERIFY(sessions.appendSegment(sid, t, QStringLiteral("work"), QStringLiteral("manual"), &err) > 0);
        QVERIFY(sessions.closeSession(sid, t.addSecs(3000), &err));

        SessionRecord closed;
        QVERIFY(sessions.sessionById(sid, &closed, &err));
        QCOMPARE(closed.open, false);
        QCOMPARE(closed.endedUtc.isValid(), true);
        QVERIFY(!sessions.activeSession(nullptr, &err));

        QList<SegmentRecord> segs = sessions.segmentsForSession(sid, &err);
        QCOMPARE(segs.size(), 1);
        QCOMPARE(segs.first().kind, QStringLiteral("work"));
        QCOMPARE(segs.first().source, QStringLiteral("manual"));
    }

    void foreignKeyIntegrityEnforced()
    {
        Database db(m_home.path() + QStringLiteral("/f.sqlite"));
        QString err;
        QVERIFY2(db.open(&err), qPrintable(err));
        QVERIFY2(Migrations::apply(db.handle(), &err), qPrintable(err));
        SessionRepository sessions(db.handle());

        const QDateTime t = QDateTime(QDate(2026, 11, 15), QTime(7, 0), Qt::UTC);
        const qint64 sid = sessions.startSession(99999, t, &err); // no such project
        QCOMPARE(sid, qint64(0));
        QVERIFY(err.contains(QStringLiteral("FOREIGN KEY")));
    }

    void backupBeforeRiskyMigration()
    {
        const QString file = m_home.path() + QStringLiteral("/g.sqlite");
        {
            Database db(file);
            QString err;
            QVERIFY2(db.open(&err), qPrintable(err));
        QVERIFY2(Migrations::apply(db.handle(), &err), qPrintable(err));
            QVERIFY2(Migrations::apply(db.handle(), &err), qPrintable(err));
        }
        QString backupPath;
        QString err;
        QVERIFY(Migrations::backupFile(file, &backupPath, &err));
        QVERIFY(QFile::exists(backupPath));
        QVERIFY(backupPath != file);
    }

private:
    QTemporaryDir m_home;
};

QTEST_MAIN(TestStorage)
#include "test_storage.moc"
