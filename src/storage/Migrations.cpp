// Kaeru — versioned schema migrations (spec §10, ADR-004)
#include "Migrations.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

namespace kaeru {

namespace {

bool exec(QSqlQuery& q, const char* sql, QString* error)
{
    if (!q.exec(QString::fromUtf8(sql))) {
        if (error)
            *error = QStringLiteral("migration failed: %1 (%2)")
                         .arg(QString::fromUtf8(sql), q.lastError().text());
        return false;
    }
    return true;
}

} // namespace

int Migrations::currentVersion(QSqlDatabase& db)
{
    QSqlQuery q(db);
    if (!q.exec(QStringLiteral("SELECT MAX(version) FROM schema_migrations")))
        return 0; // table missing → version 0
    if (!q.next())
        return 0;
    bool ok = false;
    const int v = q.value(0).toInt(&ok);
    return ok ? v : 0;
}

bool Migrations::apply(QSqlDatabase& db, QString* error)
{
    if (currentVersion(db) >= kLatestVersion)
        return true;

    QSqlQuery q(db);
    if (!exec(q, "CREATE TABLE IF NOT EXISTS schema_migrations ("
                 "version INTEGER PRIMARY KEY, applied_at TEXT NOT NULL)",
            error))
        return false;

    const int from = currentVersion(db);
    if (from < 1 && !migrateV1(db, error))
        return false;

    return true;
}

bool Migrations::migrateV1(QSqlDatabase& db, QString* error)
{
    QSqlQuery q(db);
    if (!db.transaction()) {
        if (error)
            *error = QStringLiteral("cannot begin transaction: %1").arg(db.lastError().text());
        return false;
    }

    const bool ok = exec(q, "CREATE TABLE projects ("
                            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                            "name TEXT NOT NULL COLLATE NOCASE UNIQUE,"
                            "color TEXT,"
                            "archived INTEGER NOT NULL DEFAULT 0,"
                            "created_at TEXT NOT NULL)",
                        error)
        && exec(q, "CREATE TABLE sessions ("
                   "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                   "project_id INTEGER NOT NULL REFERENCES projects(id),"
                   "started_at TEXT NOT NULL,"
                   "ended_at TEXT,"
                   "state TEXT NOT NULL DEFAULT 'open' CHECK (state IN ('open','closed')),"
                   "created_via TEXT NOT NULL DEFAULT 'manual')",
                error)
        && exec(q, "CREATE TABLE session_segments ("
                   "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                   "session_id INTEGER NOT NULL REFERENCES sessions(id) ON DELETE CASCADE,"
                   "started_at TEXT NOT NULL,"
                   "ended_at TEXT,"
                   "kind TEXT NOT NULL CHECK (kind IN ('work','break','paused','idle')),"
                   "source TEXT NOT NULL,"
                   "provenance TEXT)",
                error)
        && exec(q, "CREATE TABLE session_revisions ("
                   "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                   "session_id INTEGER REFERENCES sessions(id) ON DELETE CASCADE,"
                   "segment_id INTEGER REFERENCES session_segments(id) ON DELETE SET NULL,"
                   "revised_at TEXT NOT NULL,"
                   "reason TEXT NOT NULL,"
                   "before_json TEXT,"
                   "after_json TEXT)",
                error)
        && exec(q, "CREATE TABLE focus_sessions ("
                   "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                   "started_at TEXT NOT NULL,"
                   "ended_at TEXT,"
                   "planned_ms INTEGER NOT NULL,"
                   "achieved_ms INTEGER NOT NULL,"
                   "phase TEXT NOT NULL,"
                   "mode TEXT NOT NULL)",
                error)
        && exec(q, "CREATE TABLE recovery_checkpoints ("
                   "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                   "session_id INTEGER NOT NULL REFERENCES sessions(id) ON DELETE CASCADE,"
                   "segment_id INTEGER REFERENCES session_segments(id) ON DELETE SET NULL,"
                   "recorded_at TEXT NOT NULL,"
                   "kind TEXT NOT NULL)",
                error)
        && exec(q, "CREATE INDEX idx_segments_session ON session_segments(session_id, started_at)",
                error)
        && exec(q, "CREATE INDEX idx_segments_time ON session_segments(started_at)",
                error)
        && exec(q, "CREATE INDEX idx_sessions_state ON sessions(state)",
                error);

    if (!ok) {
        db.rollback();
        return false;
    }

    // Record the migration only on success.
    if (!q.exec(QStringLiteral("INSERT INTO schema_migrations (version, applied_at) "
                               "VALUES (1, '%1')")
                    .arg(QDateTime::currentDateTimeUtc().toString(Qt::ISODate)))
        || !db.commit()) {
        if (error)
            *error = QStringLiteral("cannot finalize migration: %1").arg(q.lastError().text());
        db.rollback();
        return false;
    }
    return true;
}

bool Migrations::backupFile(const QString& databasePath, QString* backupPath, QString* error)
{
    const QString dest = databasePath + QStringLiteral(".pre-migration-%1.bak")
                             .arg(QDateTime::currentDateTimeUtc().toString(QStringLiteral("yyyyMMdd-HHmmss")));
    QDir().mkpath(QFileInfo(databasePath).absolutePath());
    if (!QFile::copy(databasePath, dest)) {
        if (error)
            *error = QStringLiteral("backup copy failed: %1 → %2").arg(databasePath, dest);
        return false;
    }
    if (backupPath)
        *backupPath = dest;
    return true;
}

} // namespace kaeru
