// Kaeru — work session / segment repository (spec §10)
#include "SessionRepository.h"

#include <QHash>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>

#include <algorithm>

namespace kaeru {

namespace {

QDateTime utcOf(const QString& iso)
{
    return QDateTime::fromString(iso, Qt::ISODate).toUTC();
}

bool beginTx(QSqlDatabase& db, QString* error)
{
    if (!db.transaction()) {
        if (error)
            *error = QStringLiteral("cannot begin transaction: %1").arg(db.lastError().text());
        return false;
    }
    return true;
}

bool finishTx(QSqlDatabase& db, bool ok, const QSqlQuery& q, QString* error)
{
    if (!ok) {
        db.rollback();
        if (error && error->isEmpty())
            *error = q.lastError().text();
        return false;
    }
    if (!db.commit()) {
        if (error)
            *error = QStringLiteral("commit failed: %1").arg(db.lastError().text());
        db.rollback();
        return false;
    }
    return true;
}

SessionRecord readSession(const QSqlQuery& q)
{
    SessionRecord s;
    s.id = q.value(0).toLongLong();
    s.projectId = q.value(1).toLongLong();
    s.startedUtc = utcOf(q.value(2).toString());
    const QString ended = q.value(3).toString();
    if (!ended.isEmpty())
        s.endedUtc = utcOf(ended);
    s.open = q.value(4).toString() == QLatin1String("open");
    s.createdVia = q.value(5).toString();
    return s;
}

SegmentRecord readSegment(const QSqlQuery& q)
{
    SegmentRecord s;
    s.id = q.value(0).toLongLong();
    s.sessionId = q.value(1).toLongLong();
    s.startedUtc = utcOf(q.value(2).toString());
    const QString ended = q.value(3).toString();
    if (!ended.isEmpty())
        s.endedUtc = utcOf(ended);
    s.kind = q.value(4).toString();
    s.source = q.value(5).toString();
    return s;
}

} // namespace

SessionRepository::SessionRepository(QSqlDatabase db)
    : m_db(db)
{
}

QString SessionRepository::iso(const QDateTime& utc)
{
    return utc.toUTC().toString(Qt::ISODate);
}

bool SessionRepository::activeSession(SessionRecord* out, QString* error) const
{
    QSqlQuery q(m_db);
    if (!q.exec(QStringLiteral("SELECT id, project_id, started_at, ended_at, state, created_via "
                               "FROM sessions WHERE state = 'open' ORDER BY id DESC LIMIT 1"))) {
        if (error)
            *error = q.lastError().text();
        return false;
    }
    if (!q.next())
        return false;
    if (out)
        *out = readSession(q);
    return true;
}

bool SessionRepository::sessionById(qint64 id, SessionRecord* out, QString* error) const
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("SELECT id, project_id, started_at, ended_at, state, created_via "
                             "FROM sessions WHERE id = :id"));
    q.bindValue(QStringLiteral(":id"), id);
    if (!q.exec()) {
        if (error)
            *error = q.lastError().text();
        return false;
    }
    if (!q.next())
        return false;
    if (out)
        *out = readSession(q);
    return true;
}

qint64 SessionRepository::startSession(qint64 projectId, const QDateTime& atUtc, QString* error)
{
    SessionRecord existing;
    if (activeSession(&existing, error)) {
        if (error)
            *error = QStringLiteral("a session is already recording (id %1)").arg(existing.id);
        return 0;
    }
    if (!atUtc.isValid()) {
        if (error)
            *error = QStringLiteral("invalid start time");
        return 0;
    }
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("INSERT INTO sessions (project_id, started_at, state, created_via) "
                             "VALUES (:pid, :started, 'open', 'manual')"));
    q.bindValue(QStringLiteral(":pid"), projectId);
    q.bindValue(QStringLiteral(":started"), iso(atUtc));
    if (!q.exec()) {
        if (error)
            *error = q.lastError().text();
        return 0;
    }
    return q.lastInsertId().toLongLong();
}

bool SessionRepository::closeSession(qint64 sessionId, const QDateTime& atUtc, QString* error)
{
    if (!atUtc.isValid()) {
        if (error)
            *error = QStringLiteral("invalid end time");
        return false;
    }
    if (!beginTx(m_db, error))
        return false;

    QSqlQuery q(m_db);
    bool ok = true;
    // Close every still-open segment so no segment outlives its session.
    q.prepare(QStringLiteral("UPDATE session_segments SET ended_at = :end, "
                             "provenance = COALESCE(provenance, 'closed-with-session') "
                             "WHERE session_id = :sid AND ended_at IS NULL"));
    q.bindValue(QStringLiteral(":end"), iso(atUtc));
    q.bindValue(QStringLiteral(":sid"), sessionId);
    ok = q.exec();

    QSqlQuery qs(m_db);
    if (ok) {
        qs.prepare(QStringLiteral("UPDATE sessions SET ended_at = :end, state = 'closed' "
                                  "WHERE id = :sid AND state = 'open'"));
        qs.bindValue(QStringLiteral(":end"), iso(atUtc));
        qs.bindValue(QStringLiteral(":sid"), sessionId);
        ok = qs.exec();
    }

    return finishTx(m_db, ok, q, error);
}

bool SessionRepository::closeActiveSession(const QDateTime& atUtc, qint64* closedId, QString* error)
{
    SessionRecord s;
    if (!activeSession(&s, error))
        return false;
    if (closedId)
        *closedId = s.id;
    return closeSession(s.id, atUtc, error);
}

qint64 SessionRepository::appendSegment(qint64 sessionId, const QDateTime& startUtc,
                                        const QString& kind, const QString& source,
                                        QString* error)
{
    if (!startUtc.isValid()) {
        if (error)
            *error = QStringLiteral("invalid segment start");
        return 0;
    }
    if (kind != QLatin1String("work") && kind != QLatin1String("break")
        && kind != QLatin1String("paused") && kind != QLatin1String("idle")) {
        if (error)
            *error = QStringLiteral("unknown segment kind: %1").arg(kind);
        return 0;
    }
    if (!beginTx(m_db, error))
        return false;

    QSqlQuery q(m_db);
    bool ok = true;
    qint64 openSegmentId = 0;

    // Find a still-open segment and close it first (no overlap possible).
    if (ok) {
        QSqlQuery find(m_db);
        find.prepare(QStringLiteral("SELECT id FROM session_segments "
                                    "WHERE session_id = :sid AND ended_at IS NULL ORDER BY id DESC LIMIT 1"));
        find.bindValue(QStringLiteral(":sid"), sessionId);
        ok = find.exec(); // no rows is fine — first segment
        if (ok && find.next())
            openSegmentId = find.value(0).toLongLong();
    }
    if (ok && openSegmentId > 0) {
        QSqlQuery close(m_db);
        close.prepare(QStringLiteral("UPDATE session_segments SET ended_at = :end, "
                                     "provenance = 'superseded-by-next-segment' WHERE id = :seg"));
        close.bindValue(QStringLiteral(":end"), iso(startUtc));
        close.bindValue(QStringLiteral(":seg"), openSegmentId);
        ok = close.exec();
    }
    if (ok) {
        q.prepare(QStringLiteral("INSERT INTO session_segments (session_id, started_at, ended_at, kind, source) "
                                 "VALUES (:sid, :start, NULL, :kind, :source)"));
        q.bindValue(QStringLiteral(":sid"), sessionId);
        q.bindValue(QStringLiteral(":start"), iso(startUtc));
        q.bindValue(QStringLiteral(":kind"), kind);
        q.bindValue(QStringLiteral(":source"), source);
        ok = q.exec();
    }

    if (!finishTx(m_db, ok, q, error))
        return 0;
    return q.lastInsertId().toLongLong();
}

qint64 SessionRepository::closeOpenSegment(qint64 sessionId, const QDateTime& atUtc, QString* error)
{
    if (!atUtc.isValid()) {
        if (error)
            *error = QStringLiteral("invalid end time");
        return 0;
    }
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("UPDATE session_segments SET ended_at = :end, "
                             "provenance = 'closed-on-boundary' "
                             "WHERE id = (SELECT id FROM session_segments "
                             "WHERE session_id = :sid AND ended_at IS NULL ORDER BY id DESC LIMIT 1)"));
    q.bindValue(QStringLiteral(":end"), iso(atUtc));
    q.bindValue(QStringLiteral(":sid"), sessionId);
    if (!q.exec()) {
        if (error)
            *error = q.lastError().text();
        return 0;
    }
    if (q.numRowsAffected() != 1)
        return 0;
    QSqlQuery last(m_db);
    last.prepare(QStringLiteral("SELECT id FROM session_segments WHERE session_id = :sid AND ended_at = :end"));
    last.bindValue(QStringLiteral(":sid"), sessionId);
    last.bindValue(QStringLiteral(":end"), iso(atUtc));
    if (!last.exec() || !last.next())
        return 0;
    return last.value(0).toLongLong();
}

QList<SegmentRecord> SessionRepository::segmentsForSession(qint64 sessionId, QString* error) const
{
    QList<SegmentRecord> out;
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("SELECT id, session_id, started_at, ended_at, kind, source "
                             "FROM session_segments WHERE session_id = :sid ORDER BY started_at"));
    q.bindValue(QStringLiteral(":sid"), sessionId);
    if (!q.exec()) {
        if (error)
            *error = q.lastError().text();
        return out;
    }
    while (q.next())
        out.append(readSegment(q));
    return out;
}

QList<SegmentRecord> SessionRepository::segmentsBetween(const QDateTime& fromUtc, const QDateTime& toUtc,
                                                        const QString& kind, qint64 projectId,
                                                        QString* error) const
{
    QList<SegmentRecord> out;
    QString sql = QStringLiteral(
        "SELECT g.id, g.session_id, g.started_at, g.ended_at, g.kind, g.source "
        "FROM session_segments g JOIN sessions s ON s.id = g.session_id "
        "WHERE g.started_at >= :from AND g.started_at <= :to");
    if (!kind.isEmpty())
        sql += QStringLiteral(" AND g.kind = :kind");
    if (projectId > 0)
        sql += QStringLiteral(" AND s.project_id = :pid");
    sql += QStringLiteral(" ORDER BY g.started_at");

    QSqlQuery q(m_db);
    q.prepare(sql);
    q.bindValue(QStringLiteral(":from"), iso(fromUtc));
    q.bindValue(QStringLiteral(":to"), iso(toUtc));
    if (!kind.isEmpty())
        q.bindValue(QStringLiteral(":kind"), kind);
    if (projectId > 0)
        q.bindValue(QStringLiteral(":pid"), projectId);
    if (!q.exec()) {
        if (error)
            *error = q.lastError().text();
        return out;
    }
    while (q.next())
        out.append(readSegment(q));
    return out;
}

bool SessionRepository::totalsBetween(const QDateTime& fromUtc, const QDateTime& toUtc,
                                      qint64 projectId, Totals* out, QString* error) const
{
    if (out)
        *out = Totals {};
    QString sql = QStringLiteral(
        "SELECT g.kind, "
        "SUM(CAST(ROUND((julianday(g.ended_at) - julianday(g.started_at)) * 86400000) AS INTEGER)) "
        "FROM session_segments g JOIN sessions s ON s.id = g.session_id "
        "WHERE g.ended_at IS NOT NULL AND g.started_at >= :from AND g.started_at <= :to "
        "AND g.ended_at >= g.started_at");
    if (projectId > 0)
        sql += QStringLiteral(" AND s.project_id = :pid");
    sql += QStringLiteral(" GROUP BY g.kind");

    QSqlQuery q(m_db);
    q.prepare(sql);
    q.bindValue(QStringLiteral(":from"), iso(fromUtc));
    q.bindValue(QStringLiteral(":to"), iso(toUtc));
    if (projectId > 0)
        q.bindValue(QStringLiteral(":pid"), projectId);
    if (!q.exec()) {
        if (error)
            *error = q.lastError().text();
        return false;
    }
    while (q.next()) {
        const QString kind = q.value(0).toString();
        const qint64 ms = q.value(1).toLongLong();
        if (!out)
            continue;
        if (kind == QLatin1String("work"))
            out->workMs += ms;
        else if (kind == QLatin1String("break") || kind == QLatin1String("paused"))
            out->breakMs += ms;
        out->segmentCount += 0; // counted separately below
    }
    if (!out)
        return true;

    QSqlQuery count(m_db);
    QString csql = QStringLiteral(
        "SELECT COUNT(*) FROM session_segments g JOIN sessions s ON s.id = g.session_id "
        "WHERE g.ended_at IS NOT NULL AND g.started_at >= :from AND g.started_at <= :to "
        "AND g.ended_at >= g.started_at");
    if (projectId > 0)
        csql += QStringLiteral(" AND s.project_id = :pid");
    count.prepare(csql);
    count.bindValue(QStringLiteral(":from"), iso(fromUtc));
    count.bindValue(QStringLiteral(":to"), iso(toUtc));
    if (projectId > 0)
        count.bindValue(QStringLiteral(":pid"), projectId);
    if (!count.exec() || !count.next()) {
        if (error)
            *error = count.lastError().text();
        return false;
    }
    out->segmentCount = count.value(0).toInt();
    return true;
}

QList<SessionRepository::DailyTotal> SessionRepository::dailyTotals(
    const QDateTime& fromUtc, const QDateTime& toUtc, int utcOffsetSeconds, QString* error) const
{
    QList<DailyTotal> out;
    // Totals bucketed by local day using the given offset; computed from
    // closed segments only (open segments never count as work — spec §10).
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "SELECT g.kind, g.started_at, "
        "CAST(ROUND((julianday(g.ended_at) - julianday(g.started_at)) * 86400000) AS INTEGER) AS dur "
        "FROM session_segments g "
        "WHERE g.ended_at IS NOT NULL AND g.started_at >= :from AND g.started_at <= :to "
        "AND g.ended_at >= g.started_at ORDER BY g.started_at"));
    q.bindValue(QStringLiteral(":from"), iso(fromUtc));
    q.bindValue(QStringLiteral(":to"), iso(toUtc));
    if (!q.exec()) {
        if (error)
            *error = q.lastError().text();
        return out;
    }
    QHash<QDate, DailyTotal> byDay;
    while (q.next()) {
        const QString kind = q.value(0).toString();
        const QDateTime start = utcOf(q.value(1).toString());
        const QDateTime local = start.addSecs(utcOffsetSeconds);
        DailyTotal& t = byDay[local.date()];
        t.day = local.date();
        const qint64 ms = q.value(2).toLongLong();
        if (kind == QLatin1String("work"))
            t.workMs += ms;
        else if (kind == QLatin1String("break") || kind == QLatin1String("paused"))
            t.breakMs += ms;
    }
    out.reserve(byDay.size());
    for (const DailyTotal& t : byDay.values())
        out.append(t);
    std::sort(out.begin(), out.end(), [](const DailyTotal& a, const DailyTotal& b) {
        return a.day < b.day;
    });
    return out;
}

bool SessionRepository::insertFocusSession(const QDateTime& startUtc, const QDateTime& endUtc,
                                           qint64 plannedMs, qint64 achievedMs,
                                           const QString& phase, const QString& mode,
                                           QString* error)
{
    if (!startUtc.isValid() || plannedMs < 0 || achievedMs < 0) {
        if (error)
            *error = QStringLiteral("invalid focus session values");
        return false;
    }
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("INSERT INTO focus_sessions (started_at, ended_at, planned_ms, achieved_ms, phase, mode) "
                             "VALUES (:start, :end, :planned, :achieved, :phase, :mode)"));
    q.bindValue(QStringLiteral(":start"), iso(startUtc));
    q.bindValue(QStringLiteral(":end"), endUtc.isValid() ? iso(endUtc) : QVariant());
    q.bindValue(QStringLiteral(":planned"), plannedMs);
    q.bindValue(QStringLiteral(":achieved"), achievedMs);
    q.bindValue(QStringLiteral(":phase"), phase);
    q.bindValue(QStringLiteral(":mode"), mode);
    if (!q.exec()) {
        if (error)
            *error = q.lastError().text();
        return false;
    }
    return true;
}

bool SessionRepository::writeCheckpoint(qint64 sessionId, qint64 segmentId, const QDateTime& atUtc,
                                        const QString& kind, QString* error)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("INSERT INTO recovery_checkpoints (session_id, segment_id, recorded_at, kind) "
                             "VALUES (:sid, :seg, :at, :kind)"));
    q.bindValue(QStringLiteral(":sid"), sessionId);
    q.bindValue(QStringLiteral(":seg"), segmentId > 0 ? QVariant(segmentId) : QVariant());
    q.bindValue(QStringLiteral(":at"), iso(atUtc));
    q.bindValue(QStringLiteral(":kind"), kind);
    if (!q.exec()) {
        if (error)
            *error = q.lastError().text();
        return false;
    }
    return true;
}

bool SessionRepository::deleteCheckpointsForSession(qint64 sessionId, QString* error)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("DELETE FROM recovery_checkpoints WHERE session_id = :sid"));
    q.bindValue(QStringLiteral(":sid"), sessionId);
    if (!q.exec()) {
        if (error)
            *error = q.lastError().text();
        return false;
    }
    return true;
}

} // namespace kaeru
